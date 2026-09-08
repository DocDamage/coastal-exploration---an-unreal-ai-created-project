#pragma once
#include "OptionsPersistence.h"
#include <utility>

namespace coastal
{
    struct OptionsImage
    {
        OptionsSlot slot;
        std::vector<std::uint8_t> bytes;
    };
    inline OptionsImage ReadOptionsImage(bool exists, bool readSucceeded, std::vector<std::uint8_t> bytes)
    {
        if (!exists) return {};
        if (!readSucceeded) return {{OptionsStatus::ReadError, {}}, {}};
        OptionsImage result; result.bytes = std::move(bytes);
        result.slot.status = DecodeOptions(result.bytes.data(), result.bytes.size(), result.slot.record);
        return result;
    }
    enum class OptionsNotice { Defaults, Loaded, Recovered, Blocked, SessionApplied, Saved, DiskChanged, WriteUnverified };
    // Exact preference lifecycle used by the native IO wrapper. Callbacks are synchronous.
    // No auto-writes, deletes, campaign references, engine simulation, or OS process locks.
    class OptionsSession
    {
        PlayerOptions live_;
        OptionsImage observed_[2];
        OptionsSelection selection_;
        OptionsNotice notice_ = OptionsNotice::Defaults;
        bool initialized_ = false, busy_ = false;
        struct Scope
        {
            bool& busy;
            explicit Scope(bool& flag) : busy(flag) { busy = true; }
            ~Scope() { busy = false; }
            Scope(const Scope&) = delete;
            Scope& operator=(const Scope&) = delete;
        };
    public:
        const PlayerOptions& Get() const { return live_; }
        OptionsNotice Notice() const { return notice_; }
        bool IsInitialized() const { return initialized_; }
        bool UsesLegacyRecord() const
        { return initialized_ && selection_.selected >= 0 && observed_[selection_.selected].slot.record.sourceSchema == 1; }
        std::uint32_t SourceSchema() const
        { return initialized_ && selection_.selected >= 0 ? observed_[selection_.selected].slot.record.sourceSchema : 0; }
        bool NeedsFormatUpgrade() const { return SourceSchema() > 0 && SourceSchema() < OptionsSchema; }
        bool CanWrite() const { return initialized_ && !busy_ && selection_.writable; }
        template<class Read> bool Initialize(Read read)
        {
            if (initialized_ || busy_) return false;
            Scope scope(busy_);
            for (int i = 0; i < 2; ++i) observed_[i] = read(i);
            selection_ = SelectOptions(observed_[0].slot, observed_[1].slot);
            if (selection_.selected >= 0) live_ = observed_[selection_.selected].slot.record.values;
            notice_ = selection_.recovered ? OptionsNotice::Recovered :
                (selection_.selected >= 0 ? OptionsNotice::Loaded : (selection_.writable ? OptionsNotice::Defaults : OptionsNotice::Blocked));
            initialized_ = true; return true;
        }
        bool ApplySession(const PlayerOptions& values)
        {
            if (!initialized_ || busy_ || !ValidOptions(values)) return false;
            live_ = values; notice_ = OptionsNotice::SessionApplied; return true;
        }
        template<class Read, class Write> bool SaveAndApply(const PlayerOptions& values, Read read, Write write)
        {
            if (!CanWrite() || !ValidOptions(values)) return false;
            Scope scope(busy_);
            for (int i = 0; i < 2; ++i)
            {
                const auto current = read(i);
                if (current.slot.status != observed_[i].slot.status || current.bytes != observed_[i].bytes)
                { selection_.writable = false; notice_ = OptionsNotice::DiskChanged; return false; }
            }
            const OptionsRecord next{values, selection_.nextGeneration};
            std::vector<std::uint8_t> bytes;
            if (!EncodeOptions(next, bytes)) return false;
            const int target = selection_.target;
            const bool written = write(target, bytes);
            auto verified = read(target);
            if (!written || bytes != verified.bytes || verified.slot.status != OptionsStatus::Valid
                || verified.slot.record.generation != next.generation || verified.slot.record.values != values)
            {
                selection_.writable = false; notice_ = OptionsNotice::WriteUnverified; return false;
            }
            observed_[target] = std::move(verified);
            selection_ = SelectOptions(observed_[0].slot, observed_[1].slot);
            live_ = values; notice_ = OptionsNotice::Saved; return true;
        }
    };
}
