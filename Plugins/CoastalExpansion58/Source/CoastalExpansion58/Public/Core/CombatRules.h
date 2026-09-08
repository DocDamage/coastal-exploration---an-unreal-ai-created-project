#pragma once
#include <algorithm>
#include <cstdint>

namespace coastal
{
    enum class CombatDecision
    {
        Allowed,
        NotConfigured,
        NoCampaign,
        OutsideEncounter,
        InputBlocked,
        SaveBusy,
        Recovering,
        Camping,
        Swimming,
        Paused,
        ForeignAnimation,
        WrongEpoch,
        Defeated,
        Reloading,
        Cooldown,
        NoAmmo,
        MagazineFull,
        NoReserve
    };

    struct CombatGateSample
    {
        bool configured = false;
        bool activeCampaign = false;
        bool encounter = false;
        bool worldInput = false;
        bool saveBusy = false;
        bool recoveryRequired = false;
        bool returning = false;
        bool camping = false;
        bool swimming = false;
        bool paused = false;
        bool foreignAnimation = false;
        bool epochMatches = false;
        bool defeated = false;
        bool reloading = false;
        bool cooldownReady = false;
        int clip = 0;
        int clipCapacity = 0;
        int reserve = 0;
    };

    inline CombatDecision BaseCombatDecision(const CombatGateSample& s)
    {
        if (!s.configured) return CombatDecision::NotConfigured;
        if (!s.activeCampaign) return CombatDecision::NoCampaign;
        if (!s.encounter) return CombatDecision::OutsideEncounter;
        if (!s.worldInput) return CombatDecision::InputBlocked;
        if (s.saveBusy) return CombatDecision::SaveBusy;
        if (s.recoveryRequired || s.returning) return CombatDecision::Recovering;
        if (s.camping) return CombatDecision::Camping;
        if (s.swimming) return CombatDecision::Swimming;
        if (s.paused) return CombatDecision::Paused;
        if (s.foreignAnimation) return CombatDecision::ForeignAnimation;
        if (!s.epochMatches) return CombatDecision::WrongEpoch;
        if (s.defeated) return CombatDecision::Defeated;
        return CombatDecision::Allowed;
    }

    inline CombatDecision FireDecision(const CombatGateSample& s)
    {
        const auto base = BaseCombatDecision(s);
        if (base != CombatDecision::Allowed) return base;
        if (s.reloading) return CombatDecision::Reloading;
        if (!s.cooldownReady) return CombatDecision::Cooldown;
        return s.clip > 0 ? CombatDecision::Allowed : CombatDecision::NoAmmo;
    }

    inline CombatDecision ReloadDecision(const CombatGateSample& s)
    {
        const auto base = BaseCombatDecision(s);
        if (base != CombatDecision::Allowed) return base;
        if (s.reloading) return CombatDecision::Reloading;
        if (s.clip >= s.clipCapacity) return CombatDecision::MagazineFull;
        return s.reserve > 0 ? CombatDecision::Allowed : CombatDecision::NoReserve;
    }

    inline bool AcceptedAuthoritativeShot(int before, int after)
    {
        return before > 0 && after == before - 1;
    }

    struct DamageState
    {
        float health = 0;
        float shield = 0;
        bool defeated = false;
    };

    inline DamageState ApplyDamage(DamageState state, float amount)
    {
        if (state.defeated || amount <= 0) return state;
        const float absorbed = std::min(state.shield, amount);
        state.shield -= absorbed;
        state.health = std::max(0.0f, state.health - (amount - absorbed));
        state.defeated = state.health <= 0;
        return state;
    }

    struct EncounterEpoch
    {
        std::uint64_t epoch = 0;
        bool armed = false;
        bool defeated = false;

        bool Observe(std::uint64_t current)
        {
            if (current == epoch) return false;
            epoch = current;
            armed = false;
            defeated = false;
            return true;
        }
    };
}
