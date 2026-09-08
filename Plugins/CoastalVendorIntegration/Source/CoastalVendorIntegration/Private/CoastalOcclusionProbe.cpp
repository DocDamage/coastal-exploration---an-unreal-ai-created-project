#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalAGISAdapter.h"
#include "CoastalInteractionBridge.h"
#include "CoastalInteractionRelayComponent.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalWorldObject.h"
#include "FirstSignalComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

bool RunCoastalOcclusionProbe(FCoastalCampaignProbe& Probe,UCoastalHostSession* Host,FName Kind,FString& Error)
{
    const bool bDistance=Probe.Mode.StartsWith(TEXT("distance"));
    const bool bRadio=Probe.Mode.EndsWith(TEXT("_radio"));
    static TWeakObjectPtr<AActor> Blocker;
    auto Fail=[&](const TCHAR* Why){if(Blocker.IsValid())Blocker->Destroy();Blocker.Reset();Error=Why;return false;};
    auto* PC=Cast<APlayerController>(Host->GetOwner());auto* Pawn=PC?Cast<ACharacter>(PC->GetPawn()):nullptr;
    auto* Provider=Pawn?Pawn->FindComponentByClass<UCoastalAGISAdapter>():nullptr;
    auto* Bridge=Pawn?Pawn->FindComponentByClass<UCoastalInteractionBridge>():nullptr;
    auto* Relay=PC?PC->FindComponentByClass<UCoastalInteractionRelayComponent>():nullptr;
    auto* Saves=Bridge?Bridge->GetCoordinator():nullptr;
    ACoastalWorldObject* Target=nullptr;
    for(TActorIterator<ACoastalWorldObject> It(Host->GetWorld());It;++It)if(It->WorldId==(bRadio?TEXT("world.test.radio"):TEXT("world.test.battery")))Target=*It;
    if(!Saves || !Provider || !Relay || !Target || Saves->IsBusy() || Saves->IsRecoveryRequired())
        return Fail(TEXT("Reach test missing real owners"));
    auto HasApplied=[&](){return bRadio?Saves->GetMission()->ExportSnapshot().bRadioRepaired:Target->IsActive();};
    auto Key=[&](bool Down){PC->InputKey(FInputKeyEventArgs(nullptr,IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(),
        EKeys::E,Down?IE_Pressed:IE_Released,Down?1.f:0.f,false,FPlatformTime::Cycles64()));};
    if(Kind==TEXT("occlusion_arm"))
    {
        FVector Position=Target->GetActorLocation()+FVector(-145,0,0);Position.Z=100;
        Pawn->GetCharacterMovement()->StopMovementImmediately();
        if(!Pawn->TeleportTo(Position,FRotator::ZeroRotator,false,false))return Fail(TEXT("Occlusion positioning failed"));
        PC->SetControlRotation((Target->GetActorLocation()-Position).Rotation());
        if(!Bridge->PreviewInteraction(Target).bCanInteract)return Fail(TEXT("Target not initially reachable"));
        if(bRadio && (!Saves->GetMission()->ExportSnapshot().bRadioInspected || HasApplied()))
            return Fail(TEXT("Radio must be inspected and awaiting repair"));
        return true;
    }
    if(Kind==TEXT("occlusion_input"))
    {
        if(Probe.Phase==0)
        {
            PC->SetControlRotation((Target->GetActorLocation()-PC->PlayerCameraManager->GetCameraLocation()).Rotation());
            Probe.Phase=1;Probe.Next=FPlatformTime::Seconds()+0.5;return false;
        }
        const auto Offer=Relay->GetCurrentOffer();
        if(Probe.Phase==1 && Offer.WorldId!=Target->WorldId)
        {
            PC->SetControlRotation((Target->GetActorLocation()-PC->PlayerCameraManager->GetCameraLocation()).Rotation());
            Probe.Next=FPlatformTime::Seconds()+0.25;return false;
        }
        if(Probe.Phase==1)
        {
            if(!Offer.bCanInteract)return Fail(TEXT("No usable actual Hyper focus before reach fault"));
            if(bDistance)
            {
                const FVector Position=Pawn->GetActorLocation()+FVector(-200,0,0);
                Pawn->GetCharacterMovement()->StopMovementImmediately();
                if(!Pawn->TeleportTo(Position,FRotator::ZeroRotator,false,false))return Fail(TEXT("Distant positioning failed"));
            }
            else
            {
                FVector Eyes;FRotator Rotation;Pawn->GetActorEyesViewPoint(Eyes,Rotation);
                auto* Actor=Host->GetWorld()->SpawnActor<AActor>();
                if(!Actor)return Fail(TEXT("Cannot spawn test visibility blocker"));
                Blocker=Actor;
                auto* Box=NewObject<UBoxComponent>(Actor);Actor->AddInstanceComponent(Box);Actor->SetRootComponent(Box);
                Box->SetBoxExtent(FVector(20));Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
                Box->RegisterComponent();Actor->SetActorLocation((Eyes+Target->GetInteractionPoint())*0.5);
            }
            Probe.Phase=2;Probe.Next=FPlatformTime::Seconds()+0.5;return false;
        }
        if(bDistance && Offer.WorldId!=Target->WorldId)
        {
            PC->SetControlRotation((Target->GetActorLocation()-PC->PlayerCameraManager->GetCameraLocation()).Rotation());
            Probe.Next=FPlatformTime::Seconds()+0.25;return false;
        }
        if(bDistance && (!Offer.bVisible || Offer.Detail.ToString()!=TEXT("Move closer.")))
            return Fail(TEXT("Distant Hyper offer missing visible Move closer reason"));
        if((!bDistance && !Blocker.IsValid()) || Offer.bCanInteract
            || Bridge->PreviewInteraction(Target).bCanInteract)
            return Fail(TEXT("Actual Hyper/relay offer did not disable a refused target"));
        if(Provider->ExportInventory(Probe.CapacityBefore)!=ECoastalProviderResult::Ready)return Fail(TEXT("Cannot capture actual AGIS state"));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("CoastalAcceptance")/(Probe.Slot+TEXT("-refused.png")),true,false);
        Probe.CapacityGeneration=Saves->GetGeneration();Key(true);return true;
    }
    if(Kind==TEXT("occlusion_release")){Key(false);return true;}
    if(Kind==TEXT("occlusion_verify"))
    {
        FCoastalInventorySnapshot Snapshot;
        if(HasApplied() || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready
            || Snapshot.Payload!=Probe.CapacityBefore.Payload || Snapshot.Receipts.Num()!=Probe.CapacityBefore.Receipts.Num()
            || Saves->GetGeneration()!=Probe.CapacityGeneration)
            return Fail(TEXT("Refused input applied or changed inventory/save"));
        const auto Result=Bridge->TryInteract(Target);
        if(Result!=(bDistance?ECoastalActionResult::TooFar:ECoastalActionResult::Occluded) || HasApplied()
            || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready
            || Snapshot.Payload!=Probe.CapacityBefore.Payload || Snapshot.Receipts.Num()!=Probe.CapacityBefore.Receipts.Num()
            || Saves->GetGeneration()!=Probe.CapacityGeneration)
            return Fail(TEXT("Commit-time reach check did not refuse without mutation"));
        if(bDistance)
        {
            UE_LOG(LogTemp,Display,TEXT("COASTAL_DISTANCE_PASS actual_hyper_focus=1 visible_disabled_reason=1 input_refused=1 commit_too_far=1 target=%s"),*Target->WorldId.ToString());return true;
        }
        Blocker->Destroy();Blocker.Reset();
        UE_LOG(LogTemp,Display,TEXT("COASTAL_OCCLUSION_PASS real_visibility_block=1 hidden_or_disabled_hyper_offer=1 input_refused=1 commit_occluded=1 target=%s"),*Target->WorldId.ToString());return true;
    }
    if(Kind==TEXT("occlusion_collected"))
    {
        FCoastalContainerView Bag;FCoastalInventorySnapshot Snapshot;
        if(bRadio)
        {
            if(!HasApplied() || Saves->GetMission()->ExportSnapshot().bMessageHeard
                || Provider->ReadContainerView(TEXT("container.player"),Bag)!=ECoastalProviderResult::Ready || !Bag.Items.IsEmpty()
                || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready || Snapshot.Receipts.Num()!=3)
                return Fail(TEXT("Returning within clear reach did not repair exactly once"));
            UE_LOG(LogTemp,Display,TEXT("COASTAL_RADIO_REACH_RECOVERED_PASS mode=%s repaired=1 acknowledged=0 receipts=3"),*Probe.Mode);return true;
        }
        if(!Target->IsActive() || Provider->ReadContainerView(TEXT("container.player"),Bag)!=ECoastalProviderResult::Ready
            || Bag.Items.Num()!=1 || Bag.Items[0].ItemId!=TEXT("item.radio_battery") || Bag.Items[0].Quantity!=1
            || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready || Snapshot.Receipts.Num()!=1)
            return Fail(TEXT("Removing blocker did not allow one actual Hyper pickup"));
        if(bDistance)
        {
            UE_LOG(LogTemp,Display,TEXT("COASTAL_DISTANCE_RECOVERED_PASS battery=1 receipts=1"));
        }
        else
        {
            UE_LOG(LogTemp,Display,TEXT("COASTAL_OCCLUSION_RECOVERED_PASS battery=1 receipts=1"));
        }
        return true;
    }
    return Fail(TEXT("Unknown occlusion step"));
}
#endif
