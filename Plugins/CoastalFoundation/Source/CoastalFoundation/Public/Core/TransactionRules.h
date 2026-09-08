#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace coastal
{
    using Requirement = std::pair<std::string, std::int32_t>;
    inline bool ValidLogicalId(const std::string& id)
    {
        if (id.empty() || id.size() > 160 || id.front() == '.' || id.back() == '.'
            || id.find("..") != std::string::npos) return false;
        for (char c : id)
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_'))
                return false;
        return true;
    }
    // Length-prefix IDs. Reject duplicate IDs instead of silently summing overflow.
    inline bool RequirementsFingerprint(std::vector<Requirement> requirements, std::string& out)
    {
        out.clear();
        if (requirements.empty() || requirements.size() > 64) return false;
        std::sort(requirements.begin(), requirements.end());
        std::string result = "requirements.v1|";
        std::string previous;
        for (const auto& r : requirements)
        {
            if (!ValidLogicalId(r.first) || r.second <= 0 || r.first == previous) return false;
            result += std::to_string(r.first.size()) + ":" + r.first + ":" + std::to_string(r.second) + "|";
            previous = r.first;
        }
        out = result;
        return true;
    }
}
