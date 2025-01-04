//
// Created by EMCJava on 7/6/2024.
//

#pragma once

#include <Common/Stat/EffectiveStats.hpp>
#include <Common/ElementType.hpp>

#include <limits>
#include <vector>
#include <array>
#include <tuple>

// Assume all conditions are met, max buff applies
template <EchoSet Set, ElementType ETy>
struct ApplyTwoSetEffect {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.buff_multiplier += 0.1f;
    }
};

template <ElementType ETy>
struct ApplyTwoSetEffect<EchoSet::eLingeringTunes, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.percentage_attack += 0.1f;
    }
};


template <ElementType ETy>
struct ApplyTwoSetEffect<EchoSet::eMoonlitClouds, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.regen += 0.1f;
    }
};

template <ElementType ETy>
struct ApplyTwoSetEffect<EchoSet::eRejuvenatingGlow, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.heal_buff += 0.1f;
    }
};

template <ElementType ETy>
struct ApplyTwoSetEffect<EchoSet::eFrostyResolve, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.skill_buff += 0.12f;
    }
};

template <ElementType ETy>
struct ApplyTwoSetEffect<EchoSet::eEmpyreanAnthem, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.regen += 0.1f;
    }
};

template <ElementType ETy>
struct ApplyTwoSetEffect<EchoSet::eTidebreakingCourage, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.regen += 0.1f;
    }
};

// Assume all conditions are met, max buff applies
template <EchoSet Set, ElementType ETy>
struct ApplyFiveSetEffect {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.buff_multiplier += 0.3f;
    }
};

template <ElementType ETy>
struct ApplyFiveSetEffect<EchoSet::eRejuvenatingGlow, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        /* When performing the Outro Skill, the ATK of the entire team's Resonator increases by 15%, lasting 30 seconds. */
    }
};

template <ElementType ETy>
struct ApplyFiveSetEffect<EchoSet::eMoonlitClouds, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        /* After using an Outro Skill, the ATK of the next Resonator to enter the field increases by 22.5%, lasting 15 seconds. */
    }
};

template <ElementType ETy>
struct ApplyFiveSetEffect<EchoSet::eLingeringTunes, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.percentage_attack += 0.2f;
    }
};

template <ElementType ETy>
struct ApplyFiveSetEffect<EchoSet::eFrostyResolve, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.skill_buff += 0.18f;
    }
};

template <>
struct ApplyFiveSetEffect<EchoSet::eFrostyResolve, ElementType::eIceElement> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.skill_buff += 0.18f;
        Stats.buff_multiplier += 0.225f;
    }
};

template <ElementType ETy>
struct ApplyFiveSetEffect<EchoSet::eEternalRadiance, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        /*  Inflicting enemies with Spectro Frazzle increases Crit. Rate by 20% for 15s. Attacking enemies with 10 stacks of Spectro Frazzle grants 15% Spectro DMG Bonus for 15s. */
        Stats.crit_rate += 0.2f;
        Stats.buff_multiplier += 0.15f;
    }
};

template <ElementType ETy>
struct ApplyFiveSetEffect<EchoSet::eMidnightVeil, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        /* When Outro Skill is triggered, deal additional 480% Havoc DMG to surrounding enemies, considered Outro Skill DMG, and grant the incoming Resonator 15% Havoc DMG Bonus for 15s. */
    }
};

template <ElementType ETy>
struct ApplyFiveSetEffect<EchoSet::eEmpyreanAnthem, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        /* Increase the Resonator's Coordinated Attack DMG by 80%. Upon a critical hit of Coordinated Attack, increase the active Resonator's ATK by 20% for 4s. */
        // Stats.percentage_attack += 0.2f;
    }
};

template <ElementType ETy>
struct ApplyFiveSetEffect<EchoSet::eTidebreakingCourage, ETy> {
    static constexpr void Apply( EffectiveStats& Stats )
    {
        Stats.percentage_attack += 0.15f;
        // TODO: Reaching 250% Energy Regen increases all Attribute DMG by 30% for the Resonator.
        Stats.buff_multiplier += 0.3f;
    }
};

template <EchoSet Set, ElementType ETy>
inline void
ApplySetEffect( EffectiveStats& Stats, int SetCount )
{
    if ( SetCount >= 2 )
    {
        ApplyTwoSetEffect<Set, ETy>::Apply( Stats );

        if ( SetCount >= 5 )
        {
            ApplyFiveSetEffect<Set, ETy>::Apply( Stats );
        }
    }
}

using SetNameOccupation = uint32_t;
template <int Index, EchoSet Set, EchoSet... Sets>
inline void
CountSet( auto& Counter, auto& OccupationMask, EchoSet ActualSet, int NameID )
{
    if ( ActualSet == Set )
    {
        const auto Mask = (SetNameOccupation) 1 << NameID;
        if ( ~OccupationMask[ Index ] & Mask )
        {
            // Unique name
            OccupationMask[ Index ] |= Mask;
            Counter[ Index ]++;
        }
    }

    if constexpr ( sizeof...( Sets ) > 0 )
        CountSet<Index + 1, Sets...>( Counter, OccupationMask, ActualSet, NameID );
}

template <ElementType ETy, int Index, EchoSet Set, EchoSet... Sets>
inline void
ApplyAllSetByCount( EffectiveStats& Stat, auto& SetCounts )
{
    ApplySetEffect<Set, ETy>( Stat, SetCounts[ Index ] );
    if constexpr ( sizeof...( Sets ) > 0 )
        ApplyAllSetByCount<ETy, Index + 1, Sets...>( Stat, SetCounts );
}

template <ElementType ETy, EchoSet... Sets>
inline EffectiveStats
CountAndApplySets( auto&& EffectiveStatRanges, EffectiveStats CommonStats )
{
    constexpr int RelatedSetCount = sizeof...( Sets );

    std::array<int, RelatedSetCount>               SetCounts { 0 };
    std::array<SetNameOccupation, RelatedSetCount> OccupationMask { 0 };

    for ( const auto& EffectiveStat : EffectiveStatRanges )
    {
        CountSet<0, Sets...>( SetCounts, OccupationMask, EffectiveStat.Set, EffectiveStat.NameID );
        CommonStats += EffectiveStat;
    }

    ApplyAllSetByCount<ETy, 0, Sets...>( CommonStats, SetCounts );

    return CommonStats;
}

template <ElementType ETy>
inline EffectiveStats
CalculateCombinationalStat( const std::vector<EffectiveStats>& EffectiveStatRanges, const EffectiveStats& CommonStats )
{
#define SWITCH_TYPE( type ) \
    if constexpr ( ETy == ElementType::e##type##Element )

    SWITCH_TYPE( Fire )
    {
        return CountAndApplySets<ETy, EchoSet::eMoltenRift, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes, EchoSet::eRejuvenatingGlow, EchoSet::eFrostyResolve, EchoSet::eEmpyreanAnthem, EchoSet::eTidebreakingCourage>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( Air )
    {
        return CountAndApplySets<ETy, EchoSet::eSierraGale, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes, EchoSet::eRejuvenatingGlow, EchoSet::eFrostyResolve, EchoSet::eEmpyreanAnthem, EchoSet::eTidebreakingCourage>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( Ice )
    {
        return CountAndApplySets<ETy, EchoSet::eFreezingFrost, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes, EchoSet::eRejuvenatingGlow, EchoSet::eFrostyResolve, EchoSet::eEmpyreanAnthem, EchoSet::eTidebreakingCourage>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( Electric )
    {
        return CountAndApplySets<ETy, EchoSet::eVoidThunder, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes, EchoSet::eRejuvenatingGlow, EchoSet::eFrostyResolve, EchoSet::eEmpyreanAnthem, EchoSet::eTidebreakingCourage>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( Dark )
    {
        return CountAndApplySets<ETy, EchoSet::eSunSinkingEclipse, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes, EchoSet::eRejuvenatingGlow, EchoSet::eFrostyResolve, EchoSet::eMidnightVeil, EchoSet::eEmpyreanAnthem, EchoSet::eTidebreakingCourage>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( Light )
    {
        return CountAndApplySets<ETy, EchoSet::eCelestialLight, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes, EchoSet::eRejuvenatingGlow, EchoSet::eFrostyResolve, EchoSet::eEternalRadiance, EchoSet::eEmpyreanAnthem, EchoSet::eTidebreakingCourage>( EffectiveStatRanges, CommonStats );
    }

#undef SWITCH_TYPE

    std::unreachable( );
}
