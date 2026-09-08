#pragma once
#include "TransactionRules.h"
#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace coastal
{
    struct InventoryViewRow
    {
        std::string instance, definition;
        int quantity = 0, x = 0, y = 0, width = 0, height = 0;
    };
    // Validation of an ephemeral display projection, never a second inventory.
    inline bool ValidInventoryView(const std::string& expected, const std::string& actual,
        int columns, int rows, std::int64_t revision, const std::vector<InventoryViewRow>& items)
    {
        if (!ValidLogicalId(expected) || expected != actual || revision < 0
            || columns <= 0 || columns > 64 || rows <= 0 || rows > 64 || items.size() > 4096) return false;
        std::set<std::string> ids;
        std::set<int> cells;
        for (const auto& item : items)
        {
            if (item.instance.empty() || !ids.insert(item.instance).second || !ValidLogicalId(item.definition)
                || item.quantity <= 0 || item.x < 0 || item.y < 0 || item.width <= 0 || item.height <= 0
                || item.width > columns || item.height > rows
                || item.x > columns - item.width || item.y > rows - item.height) return false;
            for (int y = item.y; y < item.y + item.height; ++y)
                for (int x = item.x; x < item.x + item.width; ++x)
                    if (!cells.insert(y * columns + x).second) return false;
        }
        return true;
    }
    inline bool SaveSetFromSlot(const std::string& slot, std::string& set)
    {
        set.clear();
        const std::string prefix = "Coastal_";
        if (slot.size() <= prefix.size() + 2 || slot.compare(0, prefix.size(), prefix) != 0) return false;
        const auto suffix = slot.substr(slot.size() - 2);
        if (suffix != "_A" && suffix != "_B") return false;
        const auto candidate = slot.substr(prefix.size(), slot.size() - prefix.size() - 2);
        if (candidate.size() > 64 || !ValidLogicalId(candidate) || candidate == "none") return false;
        set = candidate; return true;
    }
}
