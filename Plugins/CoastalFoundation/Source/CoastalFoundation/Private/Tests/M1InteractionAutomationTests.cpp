#include "Misc/AutomationTest.h"
#include "CoastalInteractionRelayComponent.h"
#include "CoastalInteractionBridge.h"
#include "Core/InteractionRules.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalRelayOwnerTest,
    "Coastal.M1Interaction.InvalidOwnerFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalRelayOwnerTest::RunTest(const FString& Parameters)
{
    auto* Relay = NewObject<UCoastalInteractionRelayComponent>();
    TestFalse(TEXT("No real controller/bridge is rejected"), Relay->InitializeRelay(nullptr));
    TestFalse(TEXT("Uninitialized relay cannot publish target"), Relay->UpdateFocusedTarget(nullptr));
    TestFalse(TEXT("No invented prompt"), Relay->GetCurrentOffer().bVisible);
    TestTrue(TEXT("Uninitialized input cannot act"), Relay->SubmitExternalInput(true) == ECoastalActionResult::SuppressedInput);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalOfferFailsClosedTest,
    "Coastal.M1Interaction.ReadOnlyOfferFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalOfferFailsClosedTest::RunTest(const FString& Parameters)
{
    auto* Bridge = NewObject<UCoastalInteractionBridge>();
    const auto Offer = Bridge->PreviewInteraction(nullptr);
    TestFalse(TEXT("Unbound bridge has no world input"), Bridge->AllowsWorldInput());
    TestFalse(TEXT("Missing target is not displayed"), Offer.bVisible);
    TestFalse(TEXT("Missing target is not usable"), Offer.bCanInteract);
    TestTrue(TEXT("Empty offers compare consistently"), Offer.SamePresentation(FCoastalInteractionOffer{}));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalBridgeDebounceTest,
    "Coastal.M1Interaction.BridgeSameFrameGuard", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalBridgeDebounceTest::RunTest(const FString& Parameters)
{
    auto* Bridge = NewObject<UCoastalInteractionBridge>();
    TestTrue(TEXT("First unbound call fails honestly"), Bridge->TryInteract(nullptr) == ECoastalActionResult::NotConfigured);
    TestTrue(TEXT("Second same-frame call is suppressed"), Bridge->TryInteract(nullptr) == ECoastalActionResult::SuppressedInput);
    TestTrue(TEXT("Transcript callback cannot bypass shared guard"), Bridge->AcknowledgeTranscript(FGuid::NewGuid()) == ECoastalActionResult::SuppressedInput);
    TestTrue(TEXT("Transfer callback cannot bypass shared guard"), Bridge->TransferWithOpenStorage(FCoastalTransferRequest{}) == ECoastalActionResult::SuppressedInput);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalInteractionLeaseTest,
    "Coastal.M1Interaction.SessionAndFocusRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalInteractionLeaseTest::RunTest(const FString& Parameters)
{
    coastal::InteractionFocusLease Lease; Lease.Set(12,1,2,100);
    TestTrue(TEXT("Recent target"), Lease.Valid(12,1,2,101));
    TestFalse(TEXT("Expired target"), Lease.Valid(12,1,2,103));
    TestFalse(TEXT("Other session"), Lease.Valid(12,2,2,101));
    TestFalse(TEXT("Menu transition"), Lease.Valid(12,1,3,101));
    coastal::InteractionIntentGate Intent; Intent.Synchronize(1,0,true,1); Intent.Released();
    TestTrue(TEXT("First later press"), Intent.Press(2));
    TestFalse(TEXT("Held press does not repeat"), Intent.Press(3));
    return true;
}
#endif
