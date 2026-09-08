#include "CoastalHostSession.h"
#include "CoastalHostControls.h"
#include "CoastalLookInputComponent.h"
#include "CoastalSprintComponent.h"
#include "CoastalAudioOptionsComponent.h"
#include "CoastalAudioPlaybackComponent.h"
#include "CoastalActionAudio.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundBase.h"
#include "CoastalAGISAdapter.h"
#include "CoastalCampaignProbe.h"
#include "Misc/CommandLine.h"
#include "CoastalVendorReflection.h"
#include "CoastalInteractionBridge.h"
#include "CoastalInteractionRelayComponent.h"
#include "CoastalSessionBootstrapComponent.h"
#include "CoastalUISessionComponent.h"
#include "CoastalPlayerRecoveryComponent.h"
#include "CoastalMissionDirector.h"
#include "CoastalWorldObject.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Components/WidgetComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/TextBlock.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "TimerManager.h"
using namespace CoastalVendor;
namespace
{
template<class T> T* Add(AActor* Owner)
{ auto* C=NewObject<T>(Owner); Owner->AddInstanceComponent(C); C->RegisterComponent(); return C; }
FCoastalInteractionOffer OfferFor(ACoastalWorldObject* Target,AController* Controller)
{
    APawn* Pawn=Controller ? Controller->GetPawn().Get() : nullptr;
    auto* Bridge=Pawn ? Pawn->FindComponentByClass<UCoastalInteractionBridge>() : nullptr;
    return Bridge ? Bridge->PreviewInteraction(Target) : FCoastalInteractionOffer{};
}
}
bool UCoastalHyperFunctions::CanPresent(ACoastalWorldObject* Target,AController* Controller)
{ return OfferFor(Target,Controller).bVisible; }
void UCoastalHyperFunctions::PromptText(ACoastalWorldObject* Target,AController* Controller,FText& Primary,FText& Secondary)
{ const auto Offer=OfferFor(Target,Controller); Primary=Offer.ActionText; Secondary=Offer.Detail; }
void UCoastalHyperFunctions::PromptColor(ACoastalWorldObject* Target,AController* Controller,bool& Custom,FLinearColor& Color)
{ Custom=true; Color=OfferFor(Target,Controller).bCanInteract ? FLinearColor::White : FLinearColor(0.65f,0.65f,0.65f); }
UCoastalHostSession::UCoastalHostSession()
{
    PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.bTickEvenWhenPaused=true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
}
void UCoastalHostSession::BeginPlay()
{
    Super::BeginPlay(); Controller=Cast<APlayerController>(GetOwner());
    if(!Controller || !Controller->IsLocalController())return;
    Controller->OnPossessedPawnChanged.AddDynamic(this,&UCoastalHostSession::PawnChanged);
    GetWorld()->GetTimerManager().SetTimerForNextTick(this,&UCoastalHostSession::InitializeHost);
}
void UCoastalHostSession::PawnChanged(APawn* OldPawn,APawn* NewPawn)
{
    if(Started && OldPawn!=NewPawn)
    { FocusedProxyLabel(nullptr);Started=false; LastStatus=TEXT("Pawn changed; relaunch is required."); return; }
    if(!Attempted && NewPawn)GetWorld()->GetTimerManager().SetTimerForNextTick(this,&UCoastalHostSession::InitializeHost);
}
void UCoastalHostSession::InitializeHost()
{
    if(Attempted || !Controller || !Controller->GetPawn())return;
    Attempted=true;
    auto Fail=[&](const TCHAR* Detail){LastStatus=Detail; UE_LOG(LogTemp,Error,TEXT("COASTAL_HOST_BLOCKED: %s"),Detail);};
    auto* Character=Cast<ACharacter>(Controller->GetPawn());
    if(!Character || !Controller->PlayerInput) { Fail(TEXT("Possessed character or real player input unavailable.")); return; }
    if(Controller->FindComponentByClass<UCoastalUISessionComponent>() || Character->FindComponentByClass<UCoastalInventoryAdapter>()
        || Character->FindComponentByClass<UCoastalInteractionBridge>()) { Fail(TEXT("Existing owners must be resolved before host startup.")); return; }
    ACoastalMissionDirector* Director=nullptr;
    for(TActorIterator<ACoastalMissionDirector> It(GetWorld());It;++It)
    { if(Director){Fail(TEXT("More than one mission director."));return;} Director=*It; }
    if(!Director){Fail(TEXT("Missing mission director."));return;}
    Provider=Add<UCoastalAGISAdapter>(Character);
    if(!Provider->InitializeRealProvider())
    {
        Fail(TEXT("Actual AGIS provider initialization failed."));
        // The existing native UI explicitly supports a NotConfigured diagnostic.
        // Keep startup blocked: do not bind a replacement provider or coordinator.
        Bridge=Add<UCoastalInteractionBridge>(Character);
        UI=Add<UCoastalUISessionComponent>(Controller);
        if(!UI->InitializeUI(Director->Saves,Bridge,Character->GetActorTransform()))
            Fail(TEXT("AGIS unavailable; native diagnostic UI also failed to initialize."));
#if WITH_DEV_AUTOMATION_TESTS
        FString Mode;
        if(FParse::Value(FCommandLine::Get(),TEXT("CoastalAcceptance="),Mode) && Mode==TEXT("missing_provider"))
            Probe=MakeShared<FCoastalCampaignProbe>(Mode);
#endif
        return;
    }
    Bridge=Add<UCoastalInteractionBridge>(Character);
    Add<UCoastalPlayerRecoveryComponent>(Character);
    auto* Look=Add<UCoastalLookInputComponent>(Character);
    auto* Sprint=Add<UCoastalSprintComponent>(Character);
    auto* Controls=Add<UCoastalHostControls>(Character);
    if(!Controls->Install(Look,Sprint)){Fail(TEXT("Actual template controls signature or input readiness failed."));return;}
    Relay=Add<UCoastalInteractionRelayComponent>(Controller);
    Relay->InputOwner=ECoastalInteractInputOwner::NativeEnhancedInput;
    UI=Add<UCoastalUISessionComponent>(Controller);
    UI->bEnableDisplaySettings=true;
    if(auto* Icon=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Coastal/UI/Icons/T_RadioBattery.T_RadioBattery")))
        UI->ItemIcons.Add(TEXT("item.radio_battery"),Icon);
    if(auto* Icon=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Coastal/UI/Icons/T_MarineFuse.T_MarineFuse")))
        UI->ItemIcons.Add(TEXT("item.marine_fuse"),Icon);
    if(GetWorld()->GetMapName().EndsWith(TEXT("L_FirstSignal"))) UI->SelectedSaveSet=TEXT("first_signal_01");
    auto* Audio=Add<UCoastalAudioOptionsComponent>(Controller);
    Audio->AmbienceClass=LoadObject<USoundClass>(nullptr,TEXT("/Game/Coastal/Audio/SC_M1Ambience.SC_M1Ambience"));
    Audio->EffectsClass=LoadObject<USoundClass>(nullptr,TEXT("/Game/Coastal/Audio/SC_M1Effects.SC_M1Effects"));
    Audio->RadioClass=LoadObject<USoundClass>(nullptr,TEXT("/Game/Coastal/Audio/SC_M1Radio.SC_M1Radio"));
    Audio->bUseDedicatedSoundClasses=true;
    auto* Playback=Add<UCoastalAudioPlaybackComponent>(Controller);
    Playback->AmbienceLoop=LoadObject<USoundBase>(nullptr,TEXT("/Game/Coastal/Audio/SW_M1Ambience.SW_M1Ambience"));
    Playback->RadioTransmission=LoadObject<USoundBase>(nullptr,TEXT("/Game/Coastal/Audio/SW_M1Radio.SW_M1Radio"));
    Playback->bEnablePlayback=true;
    auto* Bootstrap=Add<UCoastalSessionBootstrapComponent>(Controller);
    Bootstrap->bInventoryCapacityFixture=GetWorld()->GetMapName().EndsWith(TEXT("L_SystemsTest_Capacity"));
    UClass* HyperClass=LoadClass<UActorComponent>(nullptr,TEXT("/Game/Coastal/Integration/BP_CoastalHyperFocus.BP_CoastalHyperFocus_C"));
    if(!HyperClass){Fail(TEXT("Generated Hyper focus-only child is missing."));return;}
    Hyper=NewObject<UActorComponent>(Character,HyperClass);
    Character->AddInstanceComponent(Hyper); Hyper->RegisterComponent();
    auto* Length=FieldAs<FDoubleProperty>(HyperClass,TEXT("Can Interact Trace length Distance"));
    if(!Length || !Hyper->FindFunction(TEXT("Can Interact Trace")) || !FieldAs<FObjectPropertyBase>(HyperClass,TEXT("Able to interact with this object")))
    {Fail(TEXT("Installed Hyper signature changed."));return;}
    Length->SetPropertyValue_InContainer(Hyper,800.0);
    HostCamera=Character->FindComponentByClass<UCameraComponent>();
    HostCameraBoom=HostCamera?Cast<USpringArmComponent>(HostCamera->GetAttachParent()):nullptr;
    auto* Front=FieldAs<FDoubleProperty>(HyperClass,TEXT("Front Offset Start Position"));
    if(!IsValid(HostCamera) || !HostCamera->IsActive() || !IsValid(HostCameraBoom) || !Front)
    {Fail(TEXT("Actual Hyper camera/boom offset signature unavailable."));return;}
    HyperFrontOffset=Front->GetPropertyValue_InContainer(Hyper);
    Relay->OnOfferChanged.AddDynamic(this,&UCoastalHostSession::OfferChanged);
    const auto Result=Bootstrap->StartTestRoom(Director,Provider,Character->GetActorTransform());
    Started=Result==ECoastalStartupResult::Started;
    if(Started)
    {
        auto* Effect=LoadObject<USoundBase>(nullptr,TEXT("/Game/Coastal/Audio/SW_M1Action.SW_M1Action"));
        const bool EffectsReady=Add<UCoastalActionAudio>(Controller)->Initialize(Bridge,Audio,Effect);
        UE_LOG(LogTemp,Display,TEXT("COASTAL_AUDIO_BINDINGS: routing=%d playback=%d effects=%d; %s"),
            Audio->IsAudioReady(),Playback->IsPlaybackReady(),EffectsReady,*Playback->LastDetail);
    }
    LastStatus=Started?TEXT("Real AGIS / Hyper host started."):TEXT("Coastal startup preflight blocked; inspect its report.");
    UE_LOG(LogTemp,Display,TEXT("COASTAL_HOST_STARTUP: %s"),*LastStatus);
#if WITH_DEV_AUTOMATION_TESTS
    FString Mode; if(Started && FParse::Value(FCommandLine::Get(),TEXT("CoastalAcceptance="),Mode))Probe=MakeShared<FCoastalCampaignProbe>(Mode);
#endif
}
void UCoastalHostSession::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    if(Probe)Probe->Tick(this);
    if(!Started || !IsValid(Hyper) || !Relay){FocusedProxyLabel(nullptr);return;}
    if(!Bridge->AllowsWorldInput())
    {
        FocusedProxyLabel(nullptr);
        Call Cancel(Hyper,TEXT("Cancel Can Interact")); Cancel.Bool(TEXT("Continue When Able To Interact Is Not Valid"),true); Cancel.Run();
        Relay->UpdateFocusedTarget(nullptr); return;
    }
    // Hyper adds requested TargetArmLength to its camera trace start. Camera
    // collision can shorten that arm without changing the requested length,
    // skipping nearby objects. Compensate only the exposed front-offset input;
    // the actual vendor still performs targeting and its line-of-sight check.
    auto* Front=FieldAs<FDoubleProperty>(Hyper->GetClass(),TEXT("Front Offset Start Position"));
    if(!IsValid(HostCamera) || !IsValid(HostCameraBoom) || !HostCamera->IsActive() || !Front)
    {FocusedProxyLabel(nullptr);Relay->UpdateFocusedTarget(nullptr);return;}
    const FVector Pivot=HostCameraBoom->GetComponentLocation()+HostCameraBoom->TargetOffset;
    const double ActualArm=FMath::Max(0.0,FVector::DotProduct(Pivot-HostCamera->GetComponentLocation(),HostCamera->GetForwardVector()));
    const double CorrectedOffset=HyperFrontOffset+ActualArm-HostCameraBoom->TargetArmLength;
    if(!FMath::IsFinite(CorrectedOffset)){FocusedProxyLabel(nullptr);Relay->UpdateFocusedTarget(nullptr);return;}
    Front->SetPropertyValue_InContainer(Hyper,CorrectedOffset);
    Call Trace(Hyper,TEXT("Can Interact Trace"));
    if(!Trace.Run()){FocusedProxyLabel(nullptr);Relay->UpdateFocusedTarget(nullptr);return;}
    auto* P=FieldAs<FObjectPropertyBase>(Hyper->GetClass(),TEXT("Able to interact with this object"));
    auto* Target=P?Cast<ACoastalWorldObject>(P->GetObjectPropertyValue_InContainer(Hyper)):nullptr;
    Relay->UpdateFocusedTarget(Target);
    // Refresh after the new real vendor sample, including unchanged focus.
    OfferChanged(Relay->GetCurrentOffer());
}
void UCoastalHostSession::OfferChanged(FCoastalInteractionOffer Offer)
{
    if(!Offer.bVisible)FocusedProxyLabel(nullptr);
    if(!IsValid(Hyper)){FocusedProxyLabel(nullptr);return;}
    auto* P=FieldAs<FObjectPropertyBase>(Hyper->GetClass(),TEXT("Active_Interact_Button_Reference"));
    auto* Button=P?Cast<AActor>(P->GetObjectPropertyValue_InContainer(Hyper)):nullptr;
    if(!Button){FocusedProxyLabel(nullptr);return;}
    auto* WidgetComponent=Button->FindComponentByClass<UWidgetComponent>();
    UObject* Widget=WidgetComponent?WidgetComponent->GetWidget():nullptr;
    if(!Widget){FocusedProxyLabel(nullptr);return;}
    // Retain Hyper's widget and focus lifetime, but place its prompt in a
    // reserved lower-screen area clear of the native objective/status HUD.
    // Deprojection follows the current viewport, including display trials.
    int32 ViewWidth=0,ViewHeight=0; Controller->GetViewportSize(ViewWidth,ViewHeight);
    FVector Origin,Direction;
    if(ViewWidth>0 && ViewHeight>0 && Controller->DeprojectScreenPositionToWorld(
        ViewWidth*0.5f,ViewHeight*0.78f,Origin,Direction))
    {
        WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
        WidgetComponent->SetPivot(FVector2D(0.5f,0.5f));
        WidgetComponent->SetWorldLocation(Origin+Direction*1000.0f);
    }
    for(const TCHAR* Name:{TEXT("Interact_Textblock"),TEXT("Interact_Textblock_Secondary")})
    {
        auto* Property=FieldAs<FObjectPropertyBase>(Widget->GetClass(),Name);
        auto* Block=Property?Cast<UTextBlock>(Property->GetObjectPropertyValue_InContainer(Widget)):nullptr;
        if(!Block)continue;
        auto Font=Block->GetFont();
        if(Font.Size!=UI->FontSize(24)){Font.Size=UI->FontSize(24);Block->SetFont(Font);}
        if(Block->GetShadowColorAndOpacity()!=FLinearColor::Black)Block->SetShadowColorAndOpacity(FLinearColor::Black);
        if(Block->GetShadowOffset()!=FVector2D(2,2))Block->SetShadowOffset(FVector2D(2,2));
        Block->SetWrapTextAt(ViewWidth*0.75f); Block->SetJustification(ETextJustify::Center);
    }
    // This vendor widget ships with a fixed A-button image. Our relay owns E /
    // gamepad FaceLeft; hide the demo image and label the actual bindings in the
    // same Hyper prompt until a device-specific icon resolver is connected.
    Call Visibility(Widget,TEXT("Update Interact Button Visibility")); Visibility.Bool(TEXT("Hide"),true); Visibility.Run();
    Call Text(Widget,TEXT("Update Interact Text"));
    auto* Primary=FieldAs<FTextProperty>(Text.Function,TEXT("Interact Text"));
    auto* Secondary=FieldAs<FTextProperty>(Text.Function,TEXT("Secondary Interact Text"));
    if(Primary && Secondary)
    {
        ACoastalWorldObject* Target=nullptr;
        if(Offer.bVisible)
            for(TActorIterator<ACoastalWorldObject> It(GetWorld());It;++It)
                if(It->WorldId==Offer.WorldId){Target=*It;break;}
        const FText Label=FText::Format(NSLOCTEXT("Coastal", "FixedInteractBindings", "[E / Gamepad X] {0}"),Offer.ActionText);
        Primary->SetPropertyValue_InContainer(Text.Data(),Label);
        const FText ObjectLabel=Target?FText::FromString(Target->DisplayLabel):FText::GetEmpty();
        const FText Detail=Target && Target->VisualMesh ? (Offer.Detail.IsEmpty() ? ObjectLabel : FText::Format(
            NSLOCTEXT("Coastal","FocusedArtNameDetail","{0}\n{1}"),ObjectLabel,Offer.Detail)) : Offer.Detail.IsEmpty()?FText::Format(
            NSLOCTEXT("Coastal","FocusedProxyName","DEV PROXY | {0}"),ObjectLabel):FText::Format(
            NSLOCTEXT("Coastal","FocusedProxyNameDetail","DEV PROXY | {0}\n{1}"),ObjectLabel,Offer.Detail);
        Secondary->SetPropertyValue_InContainer(Text.Data(),Detail);
        if(!Text.Run())Target=nullptr;
        FocusedProxyLabel(Target);
    }
    else FocusedProxyLabel(nullptr);
}
void UCoastalHostSession::FocusedProxyLabel(ACoastalWorldObject* Target)
{
    if(HiddenProxyLabel.Get()==Target)return;
    if(auto* Previous=HiddenProxyLabel.Get())
        if(IsValid(Previous->Label))Previous->Label->SetVisibility(ProxyLabelWasVisible);
    HiddenProxyLabel=Target;ProxyLabelWasVisible=false;
    if(IsValid(Target) && IsValid(Target->Label))
    {ProxyLabelWasVisible=Target->Label->IsVisible();Target->Label->SetVisibility(false);}
}
void UCoastalHostSession::EndPlay(const EEndPlayReason::Type Reason)
{
    FocusedProxyLabel(nullptr);
    if(IsValid(Controller))Controller->OnPossessedPawnChanged.RemoveDynamic(this,&UCoastalHostSession::PawnChanged);
    if(IsValid(Relay))Relay->OnOfferChanged.RemoveDynamic(this,&UCoastalHostSession::OfferChanged);
    Super::EndPlay(Reason);
}
