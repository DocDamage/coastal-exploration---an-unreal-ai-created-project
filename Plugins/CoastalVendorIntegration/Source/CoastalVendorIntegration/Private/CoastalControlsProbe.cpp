#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalHostControls.h"
#include "CoastalLookInputComponent.h"
#include "CoastalSprintComponent.h"
#include "CoastalLocalOptions.h"
#include "CoastalUISessionComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "InputKeyEventArgs.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
namespace
{
void Button(APlayerController* PC,FKey Key,bool Down)
{
    PC->InputKey(FInputKeyEventArgs(nullptr,IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(),Key,
        Down?IE_Pressed:IE_Released,Down?1.0f:0.0f,Key.IsGamepadKey(),FPlatformTime::Cycles64()));
}
void Axis(APlayerController* PC,FKey Key,float Value)
{
    PC->InputKey(FInputKeyEventArgs(nullptr,IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(),Key,
        IE_Axis,Value,Key.IsGamepadKey(),FPlatformTime::Cycles64()));
}
}
bool FCoastalCampaignProbe::RunControlsStep(UCoastalHostSession* Host,FName Kind,FString& Error)
{
    auto* PC=Cast<APlayerController>(Host->GetOwner()); auto* Pawn=PC?Cast<ACharacter>(PC->GetPawn()):nullptr;
    auto* Look=Pawn?Pawn->FindComponentByClass<UCoastalLookInputComponent>():nullptr;
    auto* Sprint=Pawn?Pawn->FindComponentByClass<UCoastalSprintComponent>():nullptr;
    auto* Move=Pawn?Pawn->GetCharacterMovement():nullptr;
    if(!Look || !Sprint || !Move || !Look->IsLookReady() || !Sprint->IsSprintReady())
    {Error=TEXT("Actual look/sprint consumer is not ready");return false;}
    const double Now=FPlatformTime::Seconds();
    auto Check=[&](bool Good,const TCHAR* Message){if(!Good)Error=Message;return Good;};
    if(Kind==TEXT("ready"))return Check(FMath::IsNearlyEqual(Move->MaxWalkSpeed,330.f),TEXT("Default walk speed not applied"));
    if(Kind==TEXT("mouse_base") || Kind==TEXT("mouse_scaled"))
    {
        if(!Phase){BeforeRotation=PC->GetControlRotation();Axis(PC,EKeys::MouseX,4);Phase=1;Next=Now+0.25;return false;}
        const double Yaw=FMath::FindDeltaAngleDegrees(BeforeRotation.Yaw,PC->GetControlRotation().Yaw);
        UE_LOG(LogTemp,Display,TEXT("COASTAL_CONTROLS_MOUSE %s yaw=%f"),*Kind.ToString(),Yaw);
        if(Kind==TEXT("mouse_base")){MouseBaseline=Yaw;return Check(Yaw>0.1 && Yaw<30,TEXT("Mouse input did not rotate camera"));}
        return Check(FMath::IsNearlyEqual(Yaw,MouseBaseline*1.25,0.15),TEXT("Mouse sensitivity not applied exactly once"));
    }
    if(Kind==TEXT("stick"))
    {
        if(!Phase){BeforeRotation=PC->GetControlRotation();Axis(PC,EKeys::Gamepad_RightX,1);Phase=1;Next=Now+0.4;return false;}
        Axis(PC,EKeys::Gamepad_RightX,0);
        const double Yaw=FMath::FindDeltaAngleDegrees(BeforeRotation.Yaw,PC->GetControlRotation().Yaw);
        UE_LOG(LogTemp,Display,TEXT("COASTAL_CONTROLS_STICK yaw=%f"),Yaw);
        return Check(Yaw>10 && Yaw<150,TEXT("Actual normalized stick input failed"));
    }
    if(Kind==TEXT("walk") || Kind==TEXT("gamepad_walk"))
    {
        if(!Phase)
        {
            PC->SetControlRotation(FRotator::ZeroRotator);BeforePosition=Pawn->GetActorLocation();
            if(Kind==TEXT("walk"))Button(PC,EKeys::W,true);else Axis(PC,EKeys::Gamepad_LeftY,1);
            Phase=1;Next=Now+0.5;return false;
        }
        if(Kind==TEXT("walk"))Button(PC,EKeys::W,false);else Axis(PC,EKeys::Gamepad_LeftY,0);
        const double Distance=FVector::Dist(BeforePosition,Pawn->GetActorLocation());
        UE_LOG(LogTemp,Display,TEXT("COASTAL_CONTROLS_MOVE %s distance_cm=%f"),*Kind.ToString(),Distance);
        return Check(Distance>65 && Distance<350,TEXT("Mapped movement did not move actual character"));
    }
    if(Kind==TEXT("sprint_hold") || Kind==TEXT("rearm"))
    {
        if(!Phase){Button(PC,EKeys::LeftShift,true);Phase=1;Next=Now+0.3;return false;}
        const bool Good=Sprint->IsSprintRequested() && FMath::IsNearlyEqual(Move->MaxWalkSpeed,520.f);
        if(Kind==TEXT("rearm"))Button(PC,EKeys::LeftShift,false);
        return Check(Good,TEXT("Fresh sprint press did not select sprint speed"));
    }
    if(Kind==TEXT("modal_held"))
    {
        Axis(PC,EKeys::Gamepad_RightX,1);BeforeRotation=PC->GetControlRotation();
        return Check(!Sprint->IsSprintRequested() && FMath::IsNearlyEqual(Move->MaxWalkSpeed,330.f),TEXT("Pause failed to cancel sprint"));
    }
    if(Kind==TEXT("held_after_modal"))
    {
        const bool Good=!Sprint->IsSprintRequested() && PC->GetControlRotation().Equals(BeforeRotation,0.05);
        Button(PC,EKeys::LeftShift,false);Axis(PC,EKeys::Gamepad_RightX,0);
        return Check(Good,TEXT("Held input resumed sprint/look after modal"));
    }
    if(Kind==TEXT("preferences"))
    {
        auto* UI=PC->FindComponentByClass<UCoastalUISessionComponent>();UCoastalLocalOptions* Profile=nullptr;
        for(TObjectIterator<UCoastalLocalOptions> It;It;++It)if(It->GetOuter()==UI)Profile=*It;
        auto* Camera=Pawn->FindComponentByClass<UCameraComponent>();
        if(!Profile || !Camera){Error=TEXT("Missing actual preference owner/camera");return false;}
        const auto& V=Profile->Get();
        FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),Slot+TEXT("-options.png")),true,false);
        return Check(V.mousePercent==125 && V.stickPercent==125 && V.fieldOfView==90 && V.textPercent==150
            && V.invertY && V.sprintToggle && FMath::IsNearlyEqual(Camera->FieldOfView,90.f),TEXT("Rendered settings failed to reach live consumers"));
    }
    if(Kind==TEXT("toggle"))
    {
        if(!Phase){Button(PC,EKeys::Gamepad_LeftThumbstick,true);Phase=1;Next=Now+0.2;return false;}
        if(Phase==1){Button(PC,EKeys::Gamepad_LeftThumbstick,false);Phase=2;Next=Now+0.3;return false;}
        if(Phase==2)
        {
            if(!Sprint->IsSprintRequested() || Move->Velocity.Size2D()>=1)
            {Error=TEXT("Toggle did not persist after release or caused auto-run");return false;}
            Button(PC,EKeys::Gamepad_LeftThumbstick,true);Phase=3;Next=Now+0.2;return false;
        }
        Button(PC,EKeys::Gamepad_LeftThumbstick,false);
        return Check(!Sprint->IsSprintRequested(),TEXT("Second deliberate toggle press failed to return to walk"));
    }
    if(Kind==TEXT("finish"))
    {
        const FString Result=TEXT("COASTAL_CONTROLS_PASS mouse/stick, WASD/gamepad movement, hold/toggle sprint, modal rearm, session preferences\n");
        UE_LOG(LogTemp,Display,TEXT("%s"),*Result);
        FFileHelper::SaveStringToFile(Result,*FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),Slot+TEXT("-controls-pass.txt")));
        return true;
    }
    Error=TEXT("Unknown control test step");return false;
}
#endif
