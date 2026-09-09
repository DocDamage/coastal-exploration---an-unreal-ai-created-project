#include "CoastalMutableCharacterComponent.h"
#include "CoastalUISessionComponent.h"
#include "CoastalSaveCoordinator.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"

bool UCoastalMutableCharacterComponent::CanEditCharacter() const
{
    return BindingValid() && bReady && !bGenerating && !bEditing
        && !Character->ActorHasTag(TEXT("Coastal.Prone"))
        && Character->GetCharacterMovement()->IsMovingOnGround();
}

bool UCoastalMutableCharacterComponent::BeginCharacterEdit()
{
    if (!CanEditCharacter()) return false;
    StopAction();
    Draft = Applied; bEditing = true; SelectedControl = 0; PreviewYaw = 0.f;
    PreviewTexture = NewObject<UTextureRenderTarget2D>(this);
    PreviewTexture->ClearColor = FLinearColor(0.035f, 0.045f, 0.055f, 1.f);
    PreviewTexture->RenderTargetFormat = RTF_RGBA8;
    PreviewTexture->InitAutoFormat(480, 480);
    Capture = NewObject<USceneCaptureComponent2D>(Character);
    Capture->SetupAttachment(Character->GetRootComponent());
    Capture->TextureTarget = PreviewTexture;
    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->bCaptureEveryFrame = false; Capture->bCaptureOnMovement = false;
    Capture->FOVAngle = 42.f;
    TArray<FEngineShowFlagsSetting> Flags;
    for (const TCHAR* Name : {TEXT("Fog"), TEXT("Atmosphere"), TEXT("EyeAdaptation"), TEXT("Bloom"), TEXT("MotionBlur"),
        TEXT("SkyLighting"), TEXT("GlobalIllumination"), TEXT("ReflectionEnvironment")})
    {
        FEngineShowFlagsSetting Flag; Flag.ShowFlagName = Name; Flag.Enabled = false; Flags.Add(Flag);
    }
    Capture->SetShowFlagSettings(Flags);
    Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
    Capture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
    Capture->PostProcessSettings.AutoExposureBias = 0.f;
    Capture->PostProcessSettings.bOverride_BloomIntensity = true;
    Capture->PostProcessSettings.BloomIntensity = 0.f;
    Character->AddInstanceComponent(Capture); Capture->RegisterComponent();
    for (int32 I = 0; I < 2; ++I)
    {
        auto* Light = NewObject<UDirectionalLightComponent>(Character);
        Light->SetupAttachment(Character->GetRootComponent());
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetLightingChannels(false, true, false);
        Light->SetIntensity(I == 0 ? 20.f : 8.f);
        Light->SetCastShadows(false);
        Light->SetIndirectLightingIntensity(0.f);
        Light->SetVolumetricScatteringIntensity(0.f);
        Character->AddInstanceComponent(Light); Light->RegisterComponent();
        PreviewLights.Add(Light);
    }
    RefreshPresentation();
    for (const auto& Part : Parts)
    {
        Capture->ShowOnlyComponent(Part);
        TArray<USceneComponent*> Children; Part->GetChildrenComponents(true, Children);
        for (auto* Child : Children) if (Child->GetOwner() == Character)
            if (auto* Primitive = Cast<UPrimitiveComponent>(Child)) Capture->ShowOnlyComponent(Primitive);
    }
    LastDetail = bCanWrite ? TEXT("Choose a setting, then adjust it. Apply and save keeps this campaign's appearance.")
        : TEXT("A damaged or incompatible appearance file was preserved. Saving is disabled; preview changes will be discarded.");
    UpdatePreview();
    return true;
}

void UCoastalMutableCharacterComponent::EndCharacterEdit()
{
    if (!bEditing) return;
    bEditing = false; Draft = Applied;
    bGenerationRequested = GeneratedSelections != Applied || bGenerating;
    if (Capture) { Capture->TextureTarget = nullptr; Capture->ClearShowOnlyComponents(); Capture->DestroyComponent(); Capture = nullptr; }
    if (PreviewTexture) { PreviewTexture->ReleaseResource(); PreviewTexture = nullptr; }
    for (const auto& Light : PreviewLights) if (Light) Light->DestroyComponent();
    PreviewLights.Reset();
    RefreshPresentation();
}

void UCoastalMutableCharacterComponent::UpdatePreview()
{
    if (!bEditing || !Capture || !Character) return;
    const double Now = FPlatformTime::Seconds();
    if (Now < NextPreviewCapture) return;
    NextPreviewCapture = Now + 0.1;
    const bool Face = SelectedControl >= 2 && SelectedControl <= 8;
    const FVector Focus = Face && GetGeneratedBody() ? GetGeneratedBody()->GetSocketLocation(TEXT("head"))
        : Character->GetActorLocation() + FVector(0, 0, -4.f);
    const FVector Offset = FRotator(0, Character->GetActorRotation().Yaw + PreviewYaw, 0).RotateVector(FVector(Face ? 100.f : 330.f, 0, Face ? 0.f : 15.f));
    Capture->SetWorldLocation(Focus + Offset);
    Capture->SetWorldRotation((-Offset).Rotation());
    for (int32 I = 0; I < PreviewLights.Num(); ++I)
        PreviewLights[I]->SetWorldRotation(FRotator(-25.f, Capture->GetComponentRotation().Yaw + (I == 0 ? 35.f : -55.f), 0));
    Capture->CaptureScene();
}

UTextureRenderTarget2D* UCoastalMutableCharacterComponent::CharacterPreview() const
{ return bEditing ? PreviewTexture.Get() : nullptr; }

void UCoastalMutableCharacterComponent::PresentCharacter(FText& Body, TArray<FCoastalUIChoice>& Choices)
{
    const auto Add = [&Choices](const TCHAR* Id, const TCHAR* Label, bool Enabled = true)
    { Choices.Add({FName(Id), FText::FromString(Label), Enabled}); };
    FString Text = TEXT("Your appearance is saved separately for this campaign. Back discards unapplied changes.\n\n");
    const bool Valid = bEditing && BindingValid() && ValidateSelections(Draft) && Definition->Controls.IsValidIndex(SelectedControl);
    if (Valid)
    {
        const auto& Control = Definition->Controls[SelectedControl];
        const int32 Value = Draft[SelectedControl];
        FString Selected = Control.Kind == ECoastalAppearanceField::Choice ? Control.Choices[Value]
            : Control.Kind == ECoastalAppearanceField::Color ? FString::Printf(TEXT("Swatch %d of %d"), Value + 1, Control.Colors.Num())
            : FString::Printf(TEXT("%.2f"), Control.Minimum + Control.Step * Value);
        Text += FString::Printf(TEXT("%s (%d of %d)\n%s\n\n"), *Control.Label.ToString(), SelectedControl + 1, Definition->Controls.Num(), *Selected);
        Add(TEXT("character_previous"), TEXT("Previous setting"));
        Add(TEXT("character_next"), TEXT("Next setting"));
        Add(TEXT("character_decrease"), TEXT("Previous option / decrease"), !bGenerating && Value > 0);
        Add(TEXT("character_increase"), TEXT("Next option / increase"), !bGenerating && Value + 1 < OptionCount(Control));
        Add(TEXT("character_left"), TEXT("Rotate view left"));
        Add(TEXT("character_right"), TEXT("Rotate view right"));
        Add(TEXT("character_restore"), TEXT("Restore saved appearance"), !bGenerating && Draft != Applied);
        Add(TEXT("character_save"), TEXT("Apply and save appearance"), !bGenerating && bReady && bCanWrite && Draft == GeneratedSelections);
    }
    Text += LastDetail;
    Body = FText::FromString(Text);
}

void UCoastalMutableCharacterComponent::CharacterCommand(FName Command)
{
    if (!bEditing || !BindingValid() || !ValidateSelections(Draft)) return;
    const int32 Count = Definition->Controls.Num();
    if (Command == TEXT("character_previous")) SelectedControl = (SelectedControl + Count - 1) % Count;
    else if (Command == TEXT("character_next")) SelectedControl = (SelectedControl + 1) % Count;
    else if (Command == TEXT("character_left")) PreviewYaw = FMath::UnwindDegrees(PreviewYaw - 30.f);
    else if (Command == TEXT("character_right")) PreviewYaw = FMath::UnwindDegrees(PreviewYaw + 30.f);
    else if (Command == TEXT("character_save")) SaveAppearance();
    else if (!bGenerating)
    {
        if (Command == TEXT("character_restore")) Draft = Applied;
        else if (Command == TEXT("character_decrease") || Command == TEXT("character_increase"))
            Draft[SelectedControl] = FMath::Clamp(Draft[SelectedControl] + (Command == TEXT("character_increase") ? 1 : -1),
                0, OptionCount(Definition->Controls[SelectedControl]) - 1);
        else return;
        bGenerationRequested = Draft != GeneratedSelections;
    }
}
