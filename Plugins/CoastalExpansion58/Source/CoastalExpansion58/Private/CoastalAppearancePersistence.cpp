#include "CoastalMutableCharacterComponent.h"
#include "CoastalSaveCoordinator.h"
#include "Kismet/GameplayStatics.h"
#include "MuCO/CustomizableObject.h"
#include "MuCO/CustomizableObjectInstance.h"

int32 UCoastalMutableCharacterComponent::OptionCount(const FCoastalAppearanceControl& Control) const
{
    if (Control.Kind == ECoastalAppearanceField::Choice) return Control.Choices.Num();
    if (Control.Kind == ECoastalAppearanceField::Color) return Control.Colors.Num();
    if (!FMath::IsFinite(Control.Minimum) || !FMath::IsFinite(Control.Maximum)
        || !FMath::IsFinite(Control.Step) || Control.Step <= 0.f || Control.Maximum < Control.Minimum) return 0;
    const float Steps = (Control.Maximum - Control.Minimum) / Control.Step;
    return FMath::IsFinite(Steps) && Steps <= 100.f ? FMath::FloorToInt(Steps + 0.001f) + 1 : 0;
}

bool UCoastalMutableCharacterComponent::ValidateSelections(const TArray<int32>& Values) const
{
    if (!IsValid(Definition) || !IsValid(Definition->DefaultInstance) || Definition->AppearanceVersion.IsNone()
        || Definition->Controls.IsEmpty() || Definition->Controls.Num() > 32 || Values.Num() != Definition->Controls.Num()) return false;
    auto* Graph = Definition->DefaultInstance->GetCustomizableObject();
    if (!IsValid(Graph) || !Graph->IsCompiled()) return false;
    TSet<FString> Names;
    for (int32 I = 0; I < Values.Num(); ++I)
    {
        const auto& Control = Definition->Controls[I];
        if (Control.Parameter.IsEmpty() || Names.Contains(Control.Parameter) || !Graph->ContainsParameter(Control.Parameter)
            || Graph->IsParameterMultidimensional(Control.Parameter) || Values[I] < 0 || Values[I] >= OptionCount(Control)) return false;
        Names.Add(Control.Parameter);
        const auto Type = Graph->GetParameterTypeByName(Control.Parameter);
        if (Control.Kind == ECoastalAppearanceField::Choice)
        {
            if (Type != EMutableParameterType::Int) return false;
            if (!Control.LinkedParameter.IsEmpty() && (!Graph->ContainsParameter(Control.LinkedParameter)
                || Graph->GetParameterTypeByName(Control.LinkedParameter) != EMutableParameterType::Int
                || Graph->IsParameterMultidimensional(Control.LinkedParameter))) return false;
            for (const auto& Choice : Control.Choices)
            {
                bool Found = false;
                for (int32 Option = 0; Option < Graph->GetEnumParameterNumValues(Control.Parameter); ++Option)
                    Found |= Graph->GetEnumParameterValue(Control.Parameter, Option) == Choice;
                if (!Found) return false;
                if (!Control.LinkedParameter.IsEmpty())
                {
                    bool LinkedFound = false;
                    for (int32 Option = 0; Option < Graph->GetEnumParameterNumValues(Control.LinkedParameter); ++Option)
                        LinkedFound |= Graph->GetEnumParameterValue(Control.LinkedParameter, Option) == Choice;
                    if (!LinkedFound) return false;
                }
            }
        }
        else if (Control.Kind == ECoastalAppearanceField::Color)
        {
            if (Type != EMutableParameterType::Color) return false;
            for (const auto& Color : Control.Colors)
                if (!FMath::IsFinite(Color.R) || !FMath::IsFinite(Color.G) || !FMath::IsFinite(Color.B)
                    || !FMath::IsFinite(Color.A) || Color.R < 0.f || Color.R > 1.f || Color.G < 0.f || Color.G > 1.f
                    || Color.B < 0.f || Color.B > 1.f || Color.A != 1.f) return false;
        }
        else if (Type != EMutableParameterType::Float) return false;
    }
    return true;
}

FString UCoastalMutableCharacterComponent::Slot(int32 Index) const
{
    return FString::Printf(TEXT("CoastalAppearance_%s_%d"), *Campaign.ToString(EGuidFormats::Digits), Index);
}

bool UCoastalMutableCharacterComponent::LoadAppearance()
{
    Applied.Reset();
    for (const auto& Control : Definition->Controls) Applied.Add(Control.DefaultSelection);
    Generation = 0; bCanWrite = true;
    UCoastalAppearanceSave* Best = nullptr;
    bool bAnyExisting = false;
    for (int32 I = 0; I < 2; ++I)
    {
        if (!UGameplayStatics::DoesSaveGameExist(Slot(I), 0)) continue;
        bAnyExisting = true;
        auto* Loaded = Cast<UCoastalAppearanceSave>(UGameplayStatics::LoadGameFromSlot(Slot(I), 0));
        if (!Loaded || Loaded->Schema != 1 || Loaded->Campaign != Campaign || Loaded->AppearanceVersion != Definition->AppearanceVersion
            || Loaded->Generation <= 0 || !ValidateSelections(Loaded->Selections))
        { bCanWrite = false; continue; }
        if (Best && Best->Generation == Loaded->Generation && Best->Selections != Loaded->Selections)
        { LastDetail = TEXT("Conflicting appearance saves. Existing files were preserved."); bCanWrite = false; return false; }
        if (!Best || Loaded->Generation > Best->Generation) Best = Loaded;
    }
    if (Best) { Applied = Best->Selections; Generation = Best->Generation; }
    else if (bAnyExisting)
    { LastDetail = TEXT("Appearance save could not be read. Existing files were preserved."); return false; }
    Draft = Applied;
    return ValidateSelections(Applied);
}

bool UCoastalMutableCharacterComponent::SaveAppearance()
{
    if (!BindingValid() || !bCanWrite || bGenerating || !bReady || Draft != GeneratedSelections
        || !ValidateSelections(Draft) || Generation == MAX_int64) return false;
    auto* Saved = NewObject<UCoastalAppearanceSave>();
    Saved->Campaign = Campaign; Saved->AppearanceVersion = Definition->AppearanceVersion;
    Saved->Generation = Generation + 1; Saved->Selections = Draft;
    const FString Target = Slot(static_cast<int32>(Saved->Generation % 2));
    TArray<uint8> Bytes, Readback;
    if (!UGameplayStatics::SaveGameToMemory(Saved, Bytes) || !UGameplayStatics::SaveDataToSlot(Bytes, Target, 0)
        || !UGameplayStatics::LoadDataFromSlot(Readback, Target, 0) || Readback != Bytes)
    { LastDetail = TEXT("Appearance save could not be verified. The previous appearance remains saved."); return false; }
    auto* Verified = Cast<UCoastalAppearanceSave>(UGameplayStatics::LoadGameFromMemory(Readback));
    if (!Verified || Verified->Campaign != Campaign || Verified->Generation != Saved->Generation || Verified->Selections != Draft)
    { LastDetail = TEXT("Appearance verification failed. The previous appearance remains saved."); return false; }
    Applied = Draft; Generation = Saved->Generation;
    LastDetail = TEXT("Appearance applied and saved for this campaign.");
    return true;
}
