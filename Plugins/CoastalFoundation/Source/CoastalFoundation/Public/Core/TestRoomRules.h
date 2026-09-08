#pragma once
#include "Core/TransactionRules.h"
#include <algorithm>
#include <array>
#include <string>
#include <unordered_set>
#include <vector>
namespace coastal
{
struct RoomObject
{
    std::string id, kind, item, journal;
    int quantity = 1;
    bool active = false;
};
struct RoomIssue { std::string code, subject, detail; };
inline const std::array<RoomObject, 7>& TestRoomManifest()
{
    static const std::array<RoomObject, 7> expected{{
        {"world.test.radio", "RADIO", "", "", 1, false},
        {"world.test.storage", "STORAGE", "", "", 1, false},
        {"world.test.door", "DOOR", "", "", 1, false},
        {"world.test.battery", "PICKUP", "item.radio_battery", "", 1, false},
        {"world.test.fuse", "PICKUP", "item.marine_fuse", "", 1, false},
        {"world.test.note", "MAINTENANCE_NOTE", "", "", 1, false},
        {"world.test.postcard", "DISCOVERY", "", "journal.first_signal.postcard", 1, false}
    }};
    return expected;
}
// Strict initial authoring contract. Map owners may supply a fixed required extension;
// callers must never derive that extension from the actors being audited or a save.
inline std::vector<RoomIssue> AuditTestRoom(const std::vector<RoomObject>& objects, int directors,
    bool capacityFixture = false, const std::vector<RoomObject>& authoredExtension = {})
{
    std::vector<RoomObject> expected(TestRoomManifest().begin(), TestRoomManifest().end());
    if (!capacityFixture) expected.insert(expected.end(), authoredExtension.begin(), authoredExtension.end());
    if (capacityFixture)
        for (int i = 1; i <= 72; ++i)
        {
            const auto digits = std::to_string(i);
            expected.push_back({"world.test.postcard." + std::string(3 - digits.size(), '0') + digits,
                "PICKUP", "item.old_postcard", "", 1, false});
        }
    std::vector<RoomIssue> issues;
    auto add = [&issues](std::string code, std::string id, std::string detail)
    { issues.push_back({std::move(code), std::move(id), std::move(detail)}); };
    if (directors != 1) add("room.director_count", "room", "Exactly one CoastalMissionDirector is required.");
    if (objects.size() > 256)
    { add("room.too_many_objects", "room", "M1 audit refuses more than 256 persistent objects."); return issues; }
    std::unordered_set<std::string> seen;
    for (const auto& object : objects)
    {
        if (!ValidLogicalId(object.id)) add("room.invalid_id", object.id, "Use the authored lowercase stable ID.");
        if (!seen.insert(object.id).second) add("room.duplicate_id", object.id, "Duplicate persistent world ID.");
        const auto found = std::find_if(expected.begin(), expected.end(), [&object](const auto& entry)
        { return entry.id == object.id; });
        if (found == expected.end())
        { add("room.unexpected_object", object.id, "This persistent object is outside the authored map manifest."); continue; }
        if (object.kind != found->kind) add("room.kind_mismatch", object.id, "Object kind does not match the manifest.");
        if (object.item != found->item || (found->kind == "PICKUP" && object.quantity != 1))
            add("room.item_mismatch", object.id, "Required pickup must grant exactly one matching part; other actors must not grant items.");
        if (object.journal != found->journal) add("room.journal_mismatch", object.id, "Journal entry differs from the authored map manifest.");
        if (object.active) add("room.not_pristine", object.id, "Startup requires initial world state, before any campaign restore.");
    }
    for (const auto& required : expected)
        if (!seen.count(required.id)) add("room.missing_object", required.id, "Required test-room actor is missing.");
    return issues;
}
}
