#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace coastal
{
    // Integrity check for accidental corruption, NOT an authenticity/security format.
    constexpr std::size_t MaxSavePayload = 16u * 1024u * 1024u;
    constexpr std::size_t EnvelopeHeaderSize = 20u;
    enum class EnvelopeStatus { Valid, Corrupt, Unsupported };
    inline std::uint32_t Crc32(const std::uint8_t* data, std::size_t count)
    {
        std::uint32_t crc = 0xffffffffu;
        for (std::size_t i = 0; i < count; ++i)
        {
            crc ^= data[i];
            for (int b = 0; b < 8; ++b)
                crc = (crc >> 1u) ^ (0xedb88320u & (0u - (crc & 1u)));
        }
        return ~crc;
    }
    inline void PutU32(std::vector<std::uint8_t>& out, std::uint32_t value)
    {
        for (unsigned b = 0; b < 32; b += 8) out.push_back(static_cast<std::uint8_t>(value >> b));
    }
    inline std::uint32_t GetU32(const std::uint8_t* data)
    {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i) value |= static_cast<std::uint32_t>(data[i]) << (8u * i);
        return value;
    }
    inline bool EncodeSave(const std::uint8_t* data, std::size_t count,
        std::vector<std::uint8_t>& out)
    {
        out.clear();
        if (!data || count == 0 || count > MaxSavePayload) return false;
        const std::uint8_t magic[] = {'C','O','A','S','T','S','A','V'};
        out.insert(out.end(), magic, magic + 8);
        PutU32(out, 1);
        PutU32(out, static_cast<std::uint32_t>(count));
        PutU32(out, Crc32(data, count));
        out.insert(out.end(), data, data + count);
        return true;
    }
    inline EnvelopeStatus DecodeSave(const std::uint8_t* data, std::size_t count,
        std::vector<std::uint8_t>& payload)
    {
        payload.clear();
        if (!data || count < EnvelopeHeaderSize || count > MaxSavePayload + EnvelopeHeaderSize)
            return EnvelopeStatus::Corrupt;
        const std::uint8_t magic[] = {'C','O','A','S','T','S','A','V'};
        for (std::size_t i = 0; i < 8; ++i) if (data[i] != magic[i]) return EnvelopeStatus::Corrupt;
        if (GetU32(data + 8) != 1) return EnvelopeStatus::Unsupported;
        const auto size = GetU32(data + 12);
        if (size == 0 || size != count - EnvelopeHeaderSize) return EnvelopeStatus::Corrupt;
        if (Crc32(data + EnvelopeHeaderSize, size) != GetU32(data + 16)) return EnvelopeStatus::Corrupt;
        payload.assign(data + EnvelopeHeaderSize, data + count);
        return EnvelopeStatus::Valid;
    }
}
