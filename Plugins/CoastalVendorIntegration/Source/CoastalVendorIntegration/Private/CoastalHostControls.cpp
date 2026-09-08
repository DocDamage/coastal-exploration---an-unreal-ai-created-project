#include "CoastalHostControls.h"
#include "CoastalVendorReflection.h"
#include "CoastalLookInputComponent.h"
#include "CoastalSprintComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

UCoastalHostControls::UCoastalHostControls()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bTickEvenWhenPaused=true;
    PrimaryComponentTick.TickGroup=TG_PrePhysics;
}
bool UCoastalHostControls::Install(UCoastalLookInputComponent* Look,UCoastalSprintComponent* Sprint)
{
    auto* Character=Cast<ACharacter>(GetOwner());
    auto* PC=Character?Cast<APlayerController>(Character->GetController()):nullptr;
    auto* PlayerInput=PC?Cast<UEnhancedPlayerInput>(PC->PlayerInput):nullptr;
    auto* Bindings=Character?Cast<UEnhancedInputComponent>(Character->InputComponent):nullptr;
    if(Installed || !IsRegistered() || !PC || !PC->IsLocalController() || !PlayerInput || !Bindings
        || !Look || Look->GetOwner()!=Character || !Sprint || Sprint->GetOwner()!=Character)return false;
    auto Action=[&](const TCHAR* Name)->UInputAction*
    {
        auto* P=CoastalVendor::FieldAs<FObjectPropertyBase>(Character->GetClass(),Name);
        return P?Cast<UInputAction>(P->GetObjectPropertyValue_InContainer(Character)):nullptr;
    };
    auto* Mouse=Action(TEXT("MouseLookAction")); auto* Stick=Action(TEXT("LookAction"));
    if(!Mouse || !Stick || Mouse==Stick || Mouse->ValueType!=EInputActionValueType::Axis2D
        || Stick->ValueType!=EInputActionValueType::Axis2D || !Stick->Modifiers.IsEmpty()
        || !Stick->Triggers.IsEmpty())return false;
    // Inspect the actual active template mapping. This bounded adapter preserves
    // its single dead zone, and refuses a different mapping instead of guessing.
    int32 StickMappings=0;
    UInputModifierDeadZone* DeadZone=nullptr;
    auto* Subsystem=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
    if(!Subsystem)return false;
    TArray<UInputMappingContext*> Contexts;
    for(const TCHAR* Name:{TEXT("DefaultMappingContexts"),TEXT("MobileExcludedMappingContexts")})
    {
        auto* Array=CoastalVendor::FieldAs<FArrayProperty>(PC->GetClass(),Name);
        auto* Inner=Array?CastField<FObjectPropertyBase>(Array->Inner):nullptr;
        if(!Inner)return false;
        FScriptArrayHelper List(Array,Array->ContainerPtrToValuePtr<void>(PC));
        for(int32 I=0;I<List.Num();++I)
            if(auto* Context=Cast<UInputMappingContext>(Inner->GetObjectPropertyValue(List.GetRawPtr(I))))
                if(Subsystem->HasMappingContext(Context))Contexts.AddUnique(Context);
    }
    for(const auto* Context:Contexts)
        for(const auto& Map:Context->GetMappings())if(Map.Action==Stick)
        {
            ++StickMappings;
            if(Map.Key!=EKeys::Gamepad_Right2D || Map.Modifiers.Num()!=1 || !Map.Triggers.IsEmpty())return false;
            DeadZone=Cast<UInputModifierDeadZone>(Map.Modifiers[0]);
            if(!DeadZone || DeadZone->GetClass()!=UInputModifierDeadZone::StaticClass())return false;
        }
    if(StickMappings!=1 || !DeadZone)return false;
    TArray<uint32> Handles; int32 MouseCount=0,StickCount=0;
    for(const auto& Binding:Bindings->GetActionEventBindings())
        if(Binding->GetAction()==Mouse || Binding->GetAction()==Stick)
        {
            if(!Binding->IsBoundToObject(Character) || Binding->GetTriggerEvent()!=ETriggerEvent::Triggered)return false;
            MouseCount+=Binding->GetAction()==Mouse; StickCount+=Binding->GetAction()==Stick;
            Handles.Add(Binding->GetHandle());
        }
    if(MouseCount!=1 || StickCount!=1)return false;
    for(uint32 Handle:Handles)Bindings->RemoveBindingByHandle(Handle);
    Controller=PC; Input=PlayerInput; MouseAction=Mouse; StickDeadZone=DeadZone;
    LookInput=Look; SprintInput=Sprint;
    LookInput->bUseHostLookEvents=true; SprintInput->bUseHostSprintEvents=true;
    AddTickPrerequisiteActor(Controller);
    SprintInput->AddTickPrerequisiteComponent(this);
    LookInput->AddTickPrerequisiteComponent(this);
    Installed=true;
    UE_LOG(LogTemp,Display,TEXT("COASTAL_CONTROLS_BOUND: separate mouse/stick; Shift/left-stick-click sprint; original look callbacks removed"));
    return true;
}
void UCoastalHostControls::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    if(!Installed)return;
    if(!IsValid(Controller) || Controller->GetPawn()!=GetOwner() || Controller->PlayerInput!=Input
        || !IsValid(LookInput) || !IsValid(SprintInput))
    { Installed=false; UE_LOG(LogTemp,Error,TEXT("Coastal controls lost fixed host; relaunch required")); return; }
    LookInput->SubmitMouseLook(Input->GetActionValue(MouseAction).Get<FVector2D>());
    // Read actual physical state even when UI blocks Enhanced Input callbacks.
    // A canceled action or modal close must never manufacture a neutral sample.
    // Enhanced Input evaluates this composite key's RawValue. The legacy 1D
    // analog getters already apply AxisConfig and would add a second dead zone.
    const FVector Raw=Input->GetRawVectorKeyValue(EKeys::Gamepad_Right2D);
    const auto Stick=StickDeadZone->ModifyRaw(Input,FInputActionValue(FVector2D(Raw.X,Raw.Y)),Delta).Get<FVector2D>();
    LookInput->SubmitStickLook(Stick);
    SprintInput->SubmitSprintInput(Controller->IsInputKeyDown(EKeys::LeftShift)
        || Controller->IsInputKeyDown(EKeys::Gamepad_LeftThumbstick));
}
void UCoastalHostControls::EndPlay(const EEndPlayReason::Type Reason)
{
    Installed=false;
    if(IsValid(SprintInput))SprintInput->RemoveTickPrerequisiteComponent(this);
    if(IsValid(LookInput))LookInput->RemoveTickPrerequisiteComponent(this);
    if(IsValid(Controller))RemoveTickPrerequisiteActor(Controller);
    Super::EndPlay(Reason);
}
