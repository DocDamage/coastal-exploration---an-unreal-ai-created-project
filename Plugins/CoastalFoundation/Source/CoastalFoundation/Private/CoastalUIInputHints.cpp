#include "CoastalUISessionComponent.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "CoastalCharacterUI.h"
#include "CoastalInteractionBridge.h"

namespace
{
// Observe real input across gameplay and UI-only menus. Never consume it or
// inject neutral/release events into movement, sprint, or interaction owners.
class FCoastalInputHints final : public IInputProcessor
{
public:
    explicit FCoastalInputHints(UCoastalUISessionComponent* Owner) : Session(Owner) {}
    virtual void Tick(float, FSlateApplication&, TSharedRef<ICursor>) override {}
    virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent& Event) override
    {
        if (!Event.IsRepeat()) Note(Event.GetKey().IsGamepadKey());
        return false;
    }
    virtual bool HandleAnalogInputEvent(FSlateApplication&, const FAnalogInputEvent& Event) override
    {
        if (Event.GetKey().IsGamepadKey() && FMath::Abs(Event.GetAnalogValue()) > 0.25f) Note(true);
        return false;
    }
    virtual bool HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent& Event) override
    {
        if (Event.GetCursorDelta().SizeSquared() > 4.0f) Note(false);
        return false;
    }
    virtual bool HandleMouseButtonDownEvent(FSlateApplication&, const FPointerEvent&) override
    { Note(false); return false; }
private:
    TWeakObjectPtr<UCoastalUISessionComponent> Session;
    void Note(bool Gamepad) { if (Session.IsValid()) Session->NoteInputDevice(Gamepad); }
};
}
void UCoastalUISessionComponent::InstallInputHints()
{
    if (!InputHints.IsValid() && FSlateApplication::IsInitialized())
    {
        InputHints = MakeShared<FCoastalInputHints>(this);
        if (!FSlateApplication::Get().RegisterInputPreProcessor(InputHints)) InputHints.Reset();
    }
}
void UCoastalUISessionComponent::RemoveInputHints()
{
    if (InputHints.IsValid() && FSlateApplication::IsInitialized())
        FSlateApplication::Get().UnregisterInputPreProcessor(InputHints);
    InputHints.Reset();
}
FText UCoastalUISessionComponent::MenuHelp() const
{
    return FText::FromString(bUsingGamepad
        ? TEXT("D-pad: focus | South button: choose | East button: back | Shoulders: scroll")
        : TEXT("Up / Down / Tab: focus | Enter: choose | Esc: back | Page Up / Down: scroll"));
}
FText UCoastalUISessionComponent::HUDControls() const
{
    FString Controls = bUsingGamepad
        ? TEXT("Menu: pause | North button: backpack | View: journal")
        : TEXT("Esc: pause | Tab: backpack | J: journal");
    if (auto* Creator = FindCoastalCharacterCreator(IsValid(Bridge) ? Bridge->GetOwner() : nullptr))
    {
        const FText Hint = Creator->CharacterActionHint(bUsingGamepad);
        if (!Hint.IsEmpty()) Controls += TEXT(" | ") + Hint.ToString();
    }
    return FText::FromString(Controls);
}
