#include "Core/OptionsPersistence.h"
#include <iostream>
#include <limits>
using namespace coastal;
static int checks = 0, failures = 0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::cerr << "line " << __LINE__ << ": " << #x << '\n'; } } while (false)
static void Set32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value)
{ for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8)); }
int main()
{
    std::vector<std::uint8_t> bytes;
    OptionsRecord r;
    CHECK(!EncodeOptions(r, bytes)); CHECK(bytes.empty());
    r.generation = 1; CHECK(EncodeOptions(r, bytes)); CHECK(bytes.size() == OptionsFileSize);
    OptionsRecord decoded;
    CHECK(DecodeOptions(bytes.data(), bytes.size(), decoded) == OptionsStatus::Valid);
    CHECK(decoded.values == r.values); CHECK(decoded.generation == 1);
    for (std::uint64_t generation : {1ull, 2ull, 0xffffffffull, 0x100000000ull, 0xffffffffffffffffull})
    {
        r.generation = generation; r.values = {300, 25, 110, 150, true};
        CHECK(EncodeOptions(r, bytes)); CHECK(DecodeOptions(bytes.data(), bytes.size(), decoded) == OptionsStatus::Valid);
        CHECK(decoded.values == r.values && decoded.generation == generation);
    }
    for (std::size_t i = 0; i < bytes.size(); ++i)
    {
        for (int bit = 0; bit < 8; ++bit)
        {
            auto damaged = bytes; damaged[i] ^= static_cast<std::uint8_t>(1u << bit);
            CHECK(DecodeOptions(damaged.data(), damaged.size(), decoded) != OptionsStatus::Valid);
            CHECK(decoded.generation == 0); // never expose a partially decoded record
        }
    }
    for (std::size_t count = 0; count < bytes.size(); ++count)
        CHECK(DecodeOptions(bytes.data(), count, decoded) == OptionsStatus::Corrupt);
    CHECK(DecodeOptions(nullptr, 4097, decoded) == OptionsStatus::Corrupt);
    auto extra = bytes; extra.push_back(0); CHECK(DecodeOptions(extra.data(), extra.size(), decoded) == OptionsStatus::Corrupt);
    std::vector<std::uint8_t> payload; CHECK(DecodeSave(bytes.data(), bytes.size(), payload) == EnvelopeStatus::Valid);
    for (auto field : {std::pair<std::size_t, std::uint32_t>{20, 0}, {20, 26}, {24, 301}, {28, 86}, {32, 90}, {36, 2}, {20, 0xffffffff}})
    {
        auto bad = payload; Set32(bad, field.first, field.second); std::vector<std::uint8_t> encoded;
        CHECK(EncodeSave(bad.data(), bad.size(), encoded));
        CHECK(DecodeOptions(encoded.data(), encoded.size(), decoded) == OptionsStatus::Corrupt);
    }
    auto future = payload; Set32(future, 8, 4); CHECK(EncodeSave(future.data(), future.size(), extra));
    CHECK(DecodeOptions(extra.data(), extra.size(), decoded) == OptionsStatus::Unsupported);
    auto zero = payload; Set32(zero, 12, 0); Set32(zero, 16, 0); CHECK(EncodeSave(zero.data(), zero.size(), extra));
    CHECK(DecodeOptions(extra.data(), extra.size(), decoded) == OptionsStatus::Corrupt);
    r.values.mousePercent = 99; CHECK(!EncodeOptions(r, bytes)); CHECK(bytes.empty());
    const OptionsSlot valid{OptionsStatus::Valid, {PlayerOptions{}, 7}};
    for (auto a : {OptionsStatus::Missing, OptionsStatus::Valid, OptionsStatus::Corrupt, OptionsStatus::Unsupported, OptionsStatus::ReadError})
    for (auto b : {OptionsStatus::Missing, OptionsStatus::Valid, OptionsStatus::Corrupt, OptionsStatus::Unsupported, OptionsStatus::ReadError})
    {
        OptionsSlot left{a, {PlayerOptions{}, 7}}, right{b, {PlayerOptions{}, 8}};
        const auto pick = SelectOptions(left, right);
        const bool blocked = a == OptionsStatus::Unsupported || b == OptionsStatus::Unsupported || a == OptionsStatus::ReadError || b == OptionsStatus::ReadError;
        const bool hasValid = a == OptionsStatus::Valid || b == OptionsStatus::Valid;
        const bool empty = a == OptionsStatus::Missing && b == OptionsStatus::Missing;
        CHECK(pick.writable == (!blocked && (hasValid || empty)));
        if (pick.selected >= 0) CHECK(pick.target != pick.selected);
    }
    CHECK(!SelectOptions(valid, valid).writable); CHECK(SelectOptions(valid, valid).selected == -1);
    auto max = valid; max.record.generation = std::numeric_limits<std::uint64_t>::max();
    CHECK(!SelectOptions(max, {}).writable); CHECK(SelectOptions(max, {}).selected == 0);
    auto bad = valid; bad.record.generation = 0; CHECK(!SelectOptions(bad, {}).writable);
    CHECK(SelectOptions({}, valid).target == 0); CHECK(SelectOptions(valid, {}).target == 1);
    std::cout << "Options codec/selection core: " << checks << " checks; " << failures << " failures\n";
    return failures ? 1 : 0;
}
