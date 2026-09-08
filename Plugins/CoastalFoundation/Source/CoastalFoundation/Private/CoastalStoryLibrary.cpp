#include "CoastalStoryLibrary.h"
#include "CoastalDestinationQuestRules.h"
#define LOCTEXT_NAMESPACE "CoastalFirstSignal"
FText UCoastalStoryLibrary::RadioTranscript()
{
    return LOCTEXT("RadioTranscript", "Coast watch, this is North Reach maintenance. The outer platform's signal lamp is still active. The service walkway is damaged\u2014approach from the marked landing. I've left the inspection log at the harbour office. Check there before setting out.");
}
FText UCoastalStoryLibrary::ObjectiveText(EFirstSignalPhase Phase)
{
    switch (Phase)
    {
    case EFirstSignalPhase::InspectRadio: return LOCTEXT("Objective_InspectRadio", "Inspect the radio at the cabin.");
    case EFirstSignalPhase::FindEquipment: return LOCTEXT("Objective_FindEquipment", "Search the old dock for a radio battery and marine fuse.");
    case EFirstSignalPhase::ReturnToRadio: return LOCTEXT("Objective_ReturnToRadio", "Return to the cabin and repair the radio.");
    case EFirstSignalPhase::ListenToRadio: return LOCTEXT("Objective_ListenToRadio", "Listen to the restored radio.");
    case EFirstSignalPhase::Complete: return LOCTEXT("Objective_Complete", "First Signal complete. A new location has been recorded in your journal.");
    default: return FText::GetEmpty();
    }
}
FText UCoastalStoryLibrary::CampaignObjective(EFirstSignalPhase Phase, const TArray<FName>& Journal)
{
    if (Phase != EFirstSignalPhase::Complete) return ObjectiveText(Phase);
    switch (FCoastalDestinationQuestRules::NextStep(true, Journal))
    {
    case INDEX_NONE: return LOCTEXT("Objective_RecordTransmission", "Record the restored transmission and North Reach lead in your journal.");
    case 0: return LOCTEXT("Objective_HarbourLog", "Read the inspection log at the Industrial Harbour.");
    case 1: return LOCTEXT("Objective_PlatformSignal", "Follow the boardwalk and check the sea platform's signal station.");
    case 2: return LOCTEXT("Objective_PowellOrder", "Read the maintenance work order at Powell Dock.");
    case 3: return LOCTEXT("Objective_Hallsands", "Inspect the evacuation marker at Hallsands.");
    case 4: return LOCTEXT("Objective_Village", "Read the register in the Medieval Italian Village courtyard.");
    case 5: return LOCTEXT("Objective_Baelo", "Inspect the survey tablet at Baelo Claudia.");
    case 6: return LOCTEXT("Objective_Prison", "Find the final duty record inside Haunted Prison.");
    default: return LOCTEXT("Objective_DestinationComplete", "Coastal Records complete. The evacuation route and shoreline survey are recorded in your journal.");
    }
}
FText UCoastalStoryLibrary::CampaignTitle(EFirstSignalPhase Phase, const TArray<FName>& Journal)
{
    if (Phase != EFirstSignalPhase::Complete) return LOCTEXT("Campaign_FirstSignal", "FIRST SIGNAL");
    const int32 Step = FCoastalDestinationQuestRules::NextStep(true, Journal);
    if (Step == INDEX_NONE) return LOCTEXT("Campaign_FirstSignal", "FIRST SIGNAL");
    if (Step < 3) return LOCTEXT("Campaign_NorthReach", "NORTH REACH");
    return LOCTEXT("Campaign_CoastalRecords", "COASTAL RECORDS");
}
FText UCoastalStoryLibrary::DiscoveryAction(FName WorldId, bool bAlreadyRead)
{
    int32 Step = INDEX_NONE;
    if (!FCoastalDestinationQuestRules::IsOptionalWorldId(WorldId, &Step))
        return bAlreadyRead ? LOCTEXT("Action_ReviewPostcard", "Review postcard") : LOCTEXT("Action_ReadPostcard", "Read postcard");
    if (bAlreadyRead)
    {
        switch (Step)
        {
        case 0: return LOCTEXT("Action_ReviewHarbour", "Review harbour inspection log");
        case 1: return LOCTEXT("Action_ReviewPlatform", "Review signal station record");
        case 2: return LOCTEXT("Action_ReviewPowell", "Review work order");
        case 3: return LOCTEXT("Action_ReviewHallsands", "Review evacuation marker");
        case 4: return LOCTEXT("Action_ReviewVillage", "Review village register");
        case 5: return LOCTEXT("Action_ReviewBaelo", "Review survey tablet");
        default: return LOCTEXT("Action_ReviewPrison", "Review prison duty record");
        }
    }
    switch (Step)
    {
    case 0: return LOCTEXT("Action_ReadHarbour", "Read harbour inspection log");
    case 1: return LOCTEXT("Action_CheckPlatform", "Check the signal station");
    case 2: return LOCTEXT("Action_ReadPowell", "Read the work order");
    case 3: return LOCTEXT("Action_InspectHallsands", "Inspect the evacuation marker");
    case 4: return LOCTEXT("Action_ReadVillage", "Read the village register");
    case 5: return LOCTEXT("Action_InspectBaelo", "Inspect the survey tablet");
    default: return LOCTEXT("Action_ReadPrison", "Read the prison duty record");
    }
}
FText UCoastalStoryLibrary::JournalText(FName EntryId)
{
    if (EntryId == TEXT("journal.first_signal.maintenance")) return LOCTEXT("Entry_0", "Cabin radio service note: the battery is missing and the marine fuse has blown. Check the old dock maintenance stores. The two parts are kept separately.");
    if (EntryId == TEXT("journal.first_signal.postcard")) return LOCTEXT("Entry_1", "The postcard shows the bay before the service walkway collapsed. Someone circled the outer signal lamp and wrote: \"Still lit after closing day.\"");
    if (EntryId == TEXT("journal.first_signal.transmission")) return RadioTranscript();
    if (EntryId == TEXT("journal.north_reach.lead")) return LOCTEXT("Entry_3", "North Reach: check the inspection log at the Industrial Harbour before approaching the outer platform.");
    if (EntryId == TEXT("journal.north_reach.harbour_log")) return LOCTEXT("Entry_HarbourLog", "The outer signal lamp is running on reserve power. A cracked switch housing has left the service crew unable to confirm its status from shore. Follow the boardwalk to the sea platform and check the signal station before ordering new parts.");
    if (EntryId == TEXT("journal.north_reach.platform_signal")) return LOCTEXT("Entry_PlatformSignal", "The reserve lamp is steady. Beside its housing, a maintenance plate records a replacement assembly delivered to Powell Dock. The platform can stay lit for now; check the dock's work order to find out why the crew never returned.");
    if (EntryId == TEXT("journal.north_reach.powell_order")) return LOCTEXT("Entry_PowellOrder", "The replacement housing reached the dock, but the crew was reassigned after a report from the old prison. Their survey notes also mention Hallsands and the village inland. You have confirmed the coastal signal route; those records may explain the abandoned settlements.");
    if (EntryId == TEXT("journal.coastal_records.hallsands")) return LOCTEXT("Entry_Hallsands", "A surviving marker gives the route used when the lower village was evacuated: follow the inland path to the tiled courtyard. Family records and the surveyor's register were carried there ahead of the storm.");
    if (EntryId == TEXT("journal.coastal_records.village")) return LOCTEXT("Entry_Village", "The register lists families received from the coast. A pencilled note says the surveyor moved the oldest maps to the ruins at Baelo Claudia, where the stone terrace offered a clear view of changes along the shore.");
    if (EntryId == TEXT("journal.coastal_records.baelo")) return LOCTEXT("Entry_Baelo", "The survey tablet compares the ancient shoreline with the present coast. A final annotation records a warning sent to the harbour and copied to the prison office. The prison's duty record should be the last surviving copy.");
    if (EntryId == TEXT("journal.coastal_records.prison")) return LOCTEXT("Entry_Prison", "The final entry confirms that the harbour warning arrived and the remaining occupants were moved inland. Supplies were left in the cells for anyone caught on the return journey. Together, the scattered records explain the evacuation route and close the coastal survey.");
    return LOCTEXT("UnknownEntry", "Development error: unknown journal entry.");
}
FText UCoastalStoryLibrary::JournalTitle(FName EntryId)
{
    if (EntryId == TEXT("journal.first_signal.maintenance")) return LOCTEXT("Title_Maintenance", "Radio service note");
    if (EntryId == TEXT("journal.first_signal.postcard")) return LOCTEXT("Title_Postcard", "Postcard from the overlook");
    if (EntryId == TEXT("journal.first_signal.transmission")) return LOCTEXT("Title_Transmission", "The North Reach transmission");
    if (EntryId == TEXT("journal.north_reach.lead")) return LOCTEXT("Title_Lead", "A lead: North Reach");
    if (EntryId == TEXT("journal.north_reach.harbour_log")) return LOCTEXT("Title_HarbourLog", "The last harbour inspection");
    if (EntryId == TEXT("journal.north_reach.platform_signal")) return LOCTEXT("Title_PlatformSignal", "A signal still burning");
    if (EntryId == TEXT("journal.north_reach.powell_order")) return LOCTEXT("Title_PowellOrder", "Powell's unfinished work");
    if (EntryId == TEXT("journal.coastal_records.hallsands")) return LOCTEXT("Title_Hallsands", "Hallsands: the evacuation mark");
    if (EntryId == TEXT("journal.coastal_records.village")) return LOCTEXT("Title_Village", "The courtyard register");
    if (EntryId == TEXT("journal.coastal_records.baelo")) return LOCTEXT("Title_Baelo", "Baelo: a changing shoreline");
    if (EntryId == TEXT("journal.coastal_records.prison")) return LOCTEXT("Title_Prison", "The last duty record");
    return LOCTEXT("Title_Unknown", "Unrecognized journal entry");
}
#undef LOCTEXT_NAMESPACE
