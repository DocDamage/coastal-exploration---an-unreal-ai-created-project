#include "CoastalContractLibrary.h"
#include "Core/TransactionRules.h"

bool UCoastalContractLibrary::MakeRequirementsFingerprint(
    const TArray<FCoastalItemRequirement>& Requirements, FString& Fingerprint)
{
    std::vector<coastal::Requirement> Values;
    for (const auto& Requirement : Requirements)
        Values.emplace_back(TCHAR_TO_UTF8(*Requirement.ItemId.ToString()), Requirement.Quantity);
    std::string Result;
    const bool bValid = coastal::RequirementsFingerprint(Values, Result);
    Fingerprint = UTF8_TO_TCHAR(Result.c_str());
    return bValid;
}
FName UCoastalContractLibrary::PickupTransactionId(FName WorldId)
{
    return WorldId.IsNone() ? NAME_None : FName(*(FString(TEXT("pickup.")) + WorldId.ToString()));
}
