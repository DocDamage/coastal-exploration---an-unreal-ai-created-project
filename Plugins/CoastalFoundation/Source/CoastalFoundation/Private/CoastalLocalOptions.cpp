#include "CoastalLocalOptions.h"
#include "Kismet/GameplayStatics.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif
#endif

FString UCoastalLocalOptions::SlotName(int32 Index)
{ return Index == 0 ? TEXT("CoastalLocalOptions_v1_A") : TEXT("CoastalLocalOptions_v1_B"); }
coastal::OptionsImage UCoastalLocalOptions::Read(int32 Index) const
{
    if (!UGameplayStatics::DoesSaveGameExist(SlotName(Index), 0)) return {};
    TArray<uint8> Bytes;
    if (!UGameplayStatics::LoadDataFromSlot(Bytes, SlotName(Index), 0)) return coastal::ReadOptionsImage(true, false, {});
    std::vector<std::uint8_t> Data;
    if (!Bytes.IsEmpty()) Data.assign(Bytes.GetData(), Bytes.GetData() + Bytes.Num());
    return coastal::ReadOptionsImage(true, true, MoveTemp(Data));
}
bool UCoastalLocalOptions::Initialize()
{ return IsInGameThread() && State.Initialize([this](int Index) { return Read(Index); }); }
bool UCoastalLocalOptions::ApplySession(const coastal::PlayerOptions& Values)
{ return IsInGameThread() && State.ApplySession(Values); }
bool UCoastalLocalOptions::SaveAndApply(const coastal::PlayerOptions& Values)
{
    if (!IsInGameThread()) return false;
#if WITH_DEV_AUTOMATION_TESTS
    FString TestMode,TestSlot,TestFault;
    FParse::Value(FCommandLine::Get(),TEXT("CoastalAcceptance="),TestMode);
    FParse::Value(FCommandLine::Get(),TEXT("CoastalAcceptanceSlot="),TestSlot);
    if(TestMode==TEXT("options_ambiguous") && TestSlot.StartsWith(TEXT("coastal_test_")))
        FParse::Value(FCommandLine::Get(),TEXT("CoastalOptionsWriteFault="),TestFault);
#if PLATFORM_WINDOWS
    HANDLE ReadbackLock=INVALID_HANDLE_VALUE;
#endif
#endif
    const bool Result=State.SaveAndApply(Values, [this](int Index) { return Read(Index); },
        [&](int Index, const std::vector<std::uint8_t>& Data)
        {
            TArray<uint8> Bytes; Bytes.Append(Data.data(), static_cast<int32>(Data.size()));
#if WITH_DEV_AUTOMATION_TESTS
            if(TestFault==TEXT("no_write"))
            {
                UE_LOG(LogTemp,Display,TEXT("COASTAL_TEST_OPTIONS_WRITE_FAULT no_write"));return false;
            }
            if(TestFault==TEXT("partial"))Bytes.SetNum(Bytes.Num()/2);
#endif
            const bool Written=UGameplayStatics::SaveDataToSlot(Bytes, SlotName(Index), 0);
#if WITH_DEV_AUTOMATION_TESTS
            if(!TestFault.IsEmpty())UE_LOG(LogTemp,Display,TEXT("COASTAL_TEST_OPTIONS_WRITE_FAULT %s actual_written=%d bytes=%d"),*TestFault,Written,Bytes.Num());
            if(Written && TestFault==TEXT("after_write"))return false;
#if PLATFORM_WINDOWS
            if(Written && TestFault==TEXT("read_lock"))
            {
                const FString Path=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SaveGames")/(SlotName(Index)+TEXT(".sav")));
                ReadbackLock=CreateFileW(*Path,GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
                UE_LOG(LogTemp,Display,TEXT("COASTAL_TEST_OPTIONS_READBACK_LOCK acquired=%d"),ReadbackLock!=INVALID_HANDLE_VALUE);
            }
#endif
#endif
            return Written;
        });
#if WITH_DEV_AUTOMATION_TESTS && PLATFORM_WINDOWS
    if(ReadbackLock!=INVALID_HANDLE_VALUE)CloseHandle(ReadbackLock);
#endif
    return Result;
}
FString UCoastalLocalOptions::Notice() const
{
    using N = coastal::OptionsNotice;
    switch (State.Notice())
    {
    case N::Defaults: return TEXT("Default options in use. No preferences file has been written yet.");
    case N::Loaded:
        if (State.UsesLegacyRecord()) return CanWrite()
            ? TEXT("M1.6 options loaded; Sprint defaults to Hold and all volumes to 100%. No file changed. An explicit verified options save upgrades the format; older plugin builds cannot read the new slot.")
            : TEXT("M1.6 options loaded with Hold sprint and default volumes; generation exhausted. Session-only changes remain available; disk writes blocked.");
        if (State.NeedsFormatUpgrade()) return CanWrite()
            ? TEXT("M1.7 options loaded; all volumes default to 100%. No file changed. Explicit options Save upgrades to schema 3; older plugins cannot read that slot.")
            : TEXT("M1.7 options loaded with default volumes; generation exhausted. Session-only changes remain available.");
        return CanWrite() ? TEXT("Saved local options loaded.") : TEXT("Saved options loaded; preference generation is exhausted. Disk writes blocked; session-only changes remain available.");
    case N::Recovered:
        if (State.UsesLegacyRecord()) return TEXT("Recovered M1.6 options with Hold sprint and default volumes from the remaining valid slot. No file changed. An explicit options save upgrades the format; older builds cannot read the new slot.");
        if (State.NeedsFormatUpgrade()) return TEXT("Recovered M1.7 options; all volumes default to 100%. No file changed. Explicit Save upgrades the format; older plugins cannot read the new slot.");
        return TEXT("Options recovered from the remaining valid slot. The damaged file was not deleted.");
    case N::Blocked: return TEXT("Options files could not be safely selected. Defaults in use; disk writes blocked. Session-only changes remain available. Back up and inspect the options files outside play.");
    case N::SessionApplied: return CanWrite() ? TEXT("Options applied for THIS SESSION ONLY. They have NOT been saved.") : TEXT("Options applied for THIS SESSION ONLY. Disk writes remain blocked; resolve the options files and relaunch before saving.");
    case N::Saved: return TEXT("Options applied and saved; preference bytes read back and verified. This is NOT a campaign save.");
    case N::DiskChanged: return TEXT("Options files changed or became unreadable. Nothing was written or applied. Relaunch before saving options.");
    case N::WriteUnverified: return TEXT("Options write could not be verified. Previous live values remain; the other slot was not touched. Relaunch to resolve disk state. Session-only apply is still available.");
    }
    return TEXT("Local options unavailable.");
}
