#include "CoastalVendorInspection.h"
#include "AdvancedShooterComponent.h"
#include "CoastalCombatActors.h"
#include "CoastalCombatComponent.h"
#include "CoastalMissionDirector.h"
#include "CoastalSaveCoordinator.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString UCoastalVendorInspection::ProbeCombatShot()
{
    UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    if (!IsInGameThread() || !World || World->GetNetMode() != NM_Standalone
        || !FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()).Contains(TEXT("/LocalHost58/")))
        return TEXT("{\"error\":\"Expected independent standalone PIE\"}");
    ACoastalMissionDirector* Director = nullptr;
    for (TActorIterator<ACoastalMissionDirector> It(World); It; ++It)
    {
        if (Director) return TEXT("{\"error\":\"Ambiguous mission director\"}");
        Director = *It;
    }
    if (!IsValid(Director) || !IsValid(Director->Saves) || !Director->Saves->HasActiveCampaign()
        || !Director->Saves->GetActiveSaveSet().ToString().StartsWith(TEXT("coastal_test_")))
        return TEXT("{\"error\":\"Expected disposable campaign\"}");
    ACharacter* Pawn = UGameplayStatics::GetPlayerCharacter(World, 0);
    auto* Combat = Pawn ? Pawn->FindComponentByClass<UCoastalCombatComponent>() : nullptr;
    auto* Shooter = Pawn ? Pawn->FindComponentByClass<UAdvancedShooterComponent>() : nullptr;
    if (!Combat || !Shooter) return TEXT("{\"error\":\"Missing combat stack\"}");
    TSet<AActor*> Before;
    for (TActorIterator<ACoastalCombatTracer> It(World); It; ++It) Before.Add(*It);
    int32 Receipts = 0;
    FVector Start = FVector::ZeroVector, End = FVector::ZeroVector;
    const FDelegateHandle Handle = Shooter->OnAuthoritativeShot.AddLambda(
        [&](AWeaponBase*, const FVector& From, const FVector& To, double)
        { ++Receipts; Start = From; End = To; });
    const int32 AmmoBefore = Combat->GetCurrentAmmo();
    const auto Result = Combat->TryFire();
    Shooter->OnAuthoritativeShot.Remove(Handle);
    int32 Tracers = 0;
    double MaxError = 0;
    for (TActorIterator<ACoastalCombatTracer> It(World); It; ++It)
    {
        if (Before.Contains(*It)) continue;
        ++Tracers;
        const auto* Mesh = It->FindComponentByClass<UStaticMeshComponent>();
        if (!Mesh) { MaxError = 1.e9; continue; }
        const FTransform Transform = Mesh->GetComponentTransform();
        const FVector ActualStart = Transform.TransformPosition(FVector(0, 0, -50));
        const FVector ActualEnd = Transform.TransformPosition(FVector(0, 0, 50));
        MaxError = FMath::Max(MaxError, FMath::Max(FVector::Distance(Start, ActualStart),
                                                 FVector::Distance(End, ActualEnd)));
    }
    auto Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("result"), StaticEnum<ECoastalCombatResult>()->GetNameStringByValue(int64(Result)));
    Report->SetNumberField(TEXT("receipts"), Receipts);
    Report->SetNumberField(TEXT("tracers"), Tracers);
    Report->SetNumberField(TEXT("max_endpoint_error_cm"), MaxError);
    Report->SetStringField(TEXT("start"), Start.ToString());
    Report->SetStringField(TEXT("end"), End.ToString());
    Report->SetNumberField(TEXT("ammo_before"), AmmoBefore);
    Report->SetNumberField(TEXT("ammo_after"), Combat->GetCurrentAmmo());
    Report->SetNumberField(TEXT("real_time"), UGameplayStatics::GetRealTimeSeconds(World));
    Report->SetNumberField(TEXT("game_time"), UGameplayStatics::GetTimeSeconds(World));
    FString Output;
    FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Output));
    return Output;
}
