#include "Core/SaveEnvelope.h"
#include "m1_test_support.h"
#include <array>
#include <random>
int main()
{
    using namespace coastal;
    TestRun t;
    const std::vector<std::uint8_t> known{'1','2','3','4','5','6','7','8','9'};
    t.Expect(Crc32(known.data(), known.size()) == 0xcbf43926u, "standard CRC32 reference");
    std::vector<std::uint8_t> bytes, decoded;
    t.Expect(EncodeSave(known.data(), known.size(), bytes), "encode succeeds");
    t.Expect(bytes.size() == EnvelopeHeaderSize + known.size(), "exact frame length");
    t.Expect(GetU32(bytes.data()+8) == 1 && GetU32(bytes.data()+12) == 9, "little-endian header");
    t.Expect(DecodeSave(bytes.data(), bytes.size(), decoded) == EnvelopeStatus::Valid && decoded == known, "round trip");
    for (std::size_t length = 0; length < bytes.size(); ++length)
        t.Expect(DecodeSave(bytes.data(), length, decoded) != EnvelopeStatus::Valid && decoded.empty(), "all truncations reject");
    auto trailing = bytes; trailing.push_back(0);
    t.Expect(DecodeSave(trailing.data(), trailing.size(), decoded) == EnvelopeStatus::Corrupt, "trailing bytes reject");
    auto future = bytes; future[8] = 2;
    t.Expect(DecodeSave(future.data(), future.size(), decoded) == EnvelopeStatus::Unsupported, "version blocks downgrade");
    for (std::size_t index = 0; index < bytes.size(); ++index)
        for (unsigned bit = 0; bit < 8; ++bit)
        {
            auto changed = bytes; changed[index] ^= static_cast<std::uint8_t>(1u << bit);
            t.Expect(DecodeSave(changed.data(), changed.size(), decoded) != EnvelopeStatus::Valid, "single-bit corruption detected");
        }
    std::mt19937 random(20260905u);
    for (std::size_t length : {1u,2u,31u,256u,4096u,65536u})
    {
        std::vector<std::uint8_t> payload(length);
        for (auto& value : payload) value = static_cast<std::uint8_t>(random());
        t.Expect(EncodeSave(payload.data(), payload.size(), bytes), "varied payload encode");
        t.Expect(DecodeSave(bytes.data(), bytes.size(), decoded) == EnvelopeStatus::Valid && payload == decoded, "varied payload roundtrip");
    }
    for (unsigned i = 0; i < 300; ++i)
    {
        std::vector<std::uint8_t> noise(i);
        for (auto& value : noise) value = static_cast<std::uint8_t>(random());
        t.Expect(DecodeSave(noise.data(), noise.size(), decoded) != EnvelopeStatus::Valid, "random garbage rejects");
    }
    t.Expect(!EncodeSave(nullptr, 0, bytes) && bytes.empty(), "null encoder rejects");
    t.Expect(!EncodeSave(known.data(), MaxSavePayload + 1, bytes), "oversize rejected before reading");
    t.Expect(DecodeSave(nullptr, MaxSavePayload + 1, decoded) == EnvelopeStatus::Corrupt, "null decoder rejects");
    return t.Finish("Save envelope core");
}
