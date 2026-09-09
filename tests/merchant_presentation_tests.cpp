#include "../Plugins/CoastalExpansion58/Source/CoastalExpansion58/Public/Core/MerchantPresentationRules.h"
#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool value, const char* detail)
{
    if (!value) { std::cerr << detail << '\n'; std::exit(1); }
}
}

int main()
{
    using namespace coastal;
    MerchantSample sample;
    sample.configured = sample.campaign = sample.epochMatches = sample.worldInput = true;
    Check(AdvanceMerchant(MerchantPhase::Dormant, sample) == MerchantPhase::Idle, "healthy wake");
    sample.worldInput = false; sample.paused = true; sample.modal = true; sample.expectedModalSeen = true;
    Check(AdvanceMerchant(MerchantPhase::AwaitJournalClose, sample) == MerchantPhase::AwaitJournalClose,
          "specific journal may hold queue while paused");
    sample.worldInput = true; sample.paused = sample.modal = false;
    Check(AdvanceMerchant(MerchantPhase::AwaitJournalClose, sample) == MerchantPhase::Pitching,
          "journal close starts pitch");
    sample.clipFinished = true;
    Check(AdvanceMerchant(MerchantPhase::Pitching, sample) == MerchantPhase::Idle, "clip returns idle");
    sample.clipFinished = false; sample.recovery = true;
    Check(AdvanceMerchant(MerchantPhase::Pitching, sample) == MerchantPhase::Dormant, "recovery cancels");
    sample.recovery = false; sample.epochMatches = false;
    Check(AdvanceMerchant(MerchantPhase::AwaitJournalClose, sample) == MerchantPhase::Dormant, "epoch cancels");
    std::cout << "merchant presentation rules passed\n";
}
