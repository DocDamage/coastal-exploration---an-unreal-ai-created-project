#pragma once
#include "PlayerOptionsRules.h"
#include "SaveEnvelope.h"
#include <limits>

namespace coastal
{
    constexpr std::size_t LegacyOptionsPayloadSize = 40;
    constexpr std::size_t OptionsPayloadSize = 60;
    constexpr std::uint32_t OptionsSchema = 3;
    constexpr std::size_t OptionsFileSize = OptionsPayloadSize + EnvelopeHeaderSize;
    enum class OptionsStatus { Missing, Valid, Corrupt, Unsupported, ReadError };
    struct OptionsRecord
    {
        PlayerOptions values; std::uint64_t generation = 0;
        std::uint32_t sourceSchema = OptionsSchema; // diagnostic only; all writes use current schema
    };
    struct OptionsSlot { OptionsStatus status = OptionsStatus::Missing; OptionsRecord record; };
    inline bool EncodeOptions(const OptionsRecord& record, std::vector<std::uint8_t>& out)
    {
        out.clear();
        if (!ValidOptions(record.values) || record.generation == 0) return false;
        std::vector<std::uint8_t> payload{'C','S','T','O','P','T','S','!'};
        PutU32(payload, OptionsSchema); PutU32(payload, static_cast<std::uint32_t>(record.generation));
        PutU32(payload, static_cast<std::uint32_t>(record.generation >> 32u));
        for (int field : {record.values.mousePercent, record.values.stickPercent,
                         record.values.fieldOfView, record.values.textPercent, record.values.invertY ? 1 : 0,
                         record.values.sprintToggle ? 1 : 0, record.values.masterPercent,
                         record.values.ambiencePercent, record.values.effectsPercent, record.values.radioPercent})
            PutU32(payload, static_cast<std::uint32_t>(field));
        return EncodeSave(payload.data(), payload.size(), out);
    }
    inline OptionsStatus DecodeOptions(const std::uint8_t* data, std::size_t count, OptionsRecord& record)
    {
        record = {};
        if (count > 4096) return OptionsStatus::Corrupt; // bound decoding before allocating an inner payload
        std::vector<std::uint8_t> payload;
        const auto envelope = DecodeSave(data, count, payload);
        if (envelope == EnvelopeStatus::Unsupported) return OptionsStatus::Unsupported;
        if (envelope != EnvelopeStatus::Valid || payload.size() < 12) return OptionsStatus::Corrupt;
        const std::uint8_t magic[] = {'C','S','T','O','P','T','S','!'};
        for (std::size_t i = 0; i < 8; ++i) if (payload[i] != magic[i]) return OptionsStatus::Corrupt;
        const auto schema = GetU32(payload.data() + 8);
        if (schema != 1 && schema != 2 && schema != OptionsSchema) return OptionsStatus::Unsupported;
        const std::size_t expectedSize = schema == 1 ? LegacyOptionsPayloadSize : (schema == 2 ? 44 : OptionsPayloadSize);
        if (payload.size() != expectedSize) return OptionsStatus::Corrupt;
        const auto at = [&payload](std::size_t offset) { return GetU32(payload.data() + offset); };
        const auto generation = static_cast<std::uint64_t>(at(12)) | (static_cast<std::uint64_t>(at(16)) << 32u);
        // Check unsigned ranges BEFORE narrowing to signed integers.
        if (!generation || at(20) > 300 || at(24) > 300 || at(28) > 110 || at(32) > 150 || at(36) > 1
            || (schema >= 2 && at(40) > 1)
            || (schema == 3 && (at(44) > 100 || at(48) > 100 || at(52) > 100 || at(56) > 100)))
            return OptionsStatus::Corrupt;
        PlayerOptions values{static_cast<int>(at(20)), static_cast<int>(at(24)),
            static_cast<int>(at(28)), static_cast<int>(at(32)), at(36) == 1,
            schema >= 2 && at(40) == 1};
        if (schema == 3)
        {
            values.masterPercent = static_cast<int>(at(44)); values.ambiencePercent = static_cast<int>(at(48));
            values.effectsPercent = static_cast<int>(at(52)); values.radioPercent = static_cast<int>(at(56));
        }
        if (!ValidOptions(values)) return OptionsStatus::Corrupt;
        record = {values, generation, schema}; return OptionsStatus::Valid;
    }
    struct OptionsSelection
    {
        int selected = -1, target = -1;
        std::uint64_t nextGeneration = 0;
        bool recovered = false, writable = false;
    };
    inline OptionsSelection SelectOptions(const OptionsSlot& a, const OptionsSlot& b)
    {
        const auto blocked = [](OptionsStatus s) { return s == OptionsStatus::Unsupported || s == OptionsStatus::ReadError; };
        if (blocked(a.status) || blocked(b.status)) return {};
        const bool av = a.status == OptionsStatus::Valid, bv = b.status == OptionsStatus::Valid;
        if (av && (!a.record.generation || !ValidOptions(a.record.values))) return {};
        if (bv && (!b.record.generation || !ValidOptions(b.record.values))) return {};
        if (av && bv && a.record.generation == b.record.generation) return {}; // never guess on a conflict
        if (!av && !bv)
        {
            if (a.status == OptionsStatus::Missing && b.status == OptionsStatus::Missing) return {-1, 0, 1, false, true};
            return {}; // existing damaged files are not silently reset or deleted
        }
        const int best = av && (!bv || a.record.generation > b.record.generation) ? 0 : 1;
        const auto generation = best == 0 ? a.record.generation : b.record.generation;
        const bool writable = generation != std::numeric_limits<std::uint64_t>::max();
        return {best, writable ? 1 - best : -1, writable ? generation + 1 : 0,
            a.status == OptionsStatus::Corrupt || b.status == OptionsStatus::Corrupt, writable};
    }
}
