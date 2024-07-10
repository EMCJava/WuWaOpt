//
// Created by EMCJava on 7/6/2024.
//

#pragma once

#include "FullStats.hpp"
#include "OptUtil.hpp"

#include <array>
#include <tuple>
#include <limits>

// Assume all conditions are met, max buff applies
template <EchoSet Set>
inline void
ApplyTwoSetEffect( EffectiveStats& Stats )
{
    Stats.buff_multiplier += 0.1f;
}

template <>
inline void
ApplyTwoSetEffect<eLingeringTunes>( EffectiveStats& Stats )
{
    Stats.percentage_attack += 0.1f;
}

template <>
inline void
ApplyTwoSetEffect<eMoonlitClouds>( EffectiveStats& Stats )
{
    Stats.regen += 0.1f;
}

template <>
inline void
ApplyTwoSetEffect<eRejuvenatingGlow>( EffectiveStats& Stats )
{
    /* Healing Bonus increased by 10%. */
}

// Assume all conditions are met, max buff applies
template <EchoSet Set>
inline void
ApplyFiveSetEffect( EffectiveStats& Stats )
{
    Stats.buff_multiplier += 0.3f;
}

template <>
inline void
ApplyFiveSetEffect<eRejuvenatingGlow>( EffectiveStats& Stats )
{
    /* When performing the Outro Skill, the ATK of the entire team's Resonator increases by 15%, lasting 30 seconds. */
}

template <>
inline void
ApplyFiveSetEffect<eMoonlitClouds>( EffectiveStats& Stats )
{
    /* After using an Outro Skill, the ATK of the next Resonator to enter the field increases by 22.5%, lasting 15 seconds. */
}

template <>
inline void
ApplyFiveSetEffect<eLingeringTunes>( EffectiveStats& Stats )
{
    Stats.percentage_attack += 0.2f;
}

template <EchoSet Set>
inline void
ApplySetEffect( EffectiveStats& Stats, int SetCount )
{
    if ( SetCount >= 2 )
    {
        ApplyTwoSetEffect<Set>( Stats );

        if ( SetCount >= 5 )
        {
            ApplyFiveSetEffect<Set>( Stats );
        }
    }
}

template <int Index, EchoSet Set, EchoSet... Sets>
inline void
CountSet( auto& Counter, char ActualSet )
{
    if ( ActualSet == Set ) Counter[ Index ]++;
    if constexpr ( sizeof...( Sets ) > 0 )
        CountSet<Index + 1, Sets...>( Counter, ActualSet );
}

template <int Index, EchoSet Set, EchoSet... Sets>
inline void
ApplyAllSetByCount( EffectiveStats& Stat, auto& SetCounts )
{
    ApplySetEffect<Set>( Stat, SetCounts[ Index ] );
    if constexpr ( sizeof...( Sets ) > 0 )
        ApplyAllSetByCount<Index + 1, Sets...>( Stat, SetCounts );
}

template <EchoSet... Sets>
inline EffectiveStats
CountAndApplySets( auto&& EffectiveStatRanges, EffectiveStats CommonStats )
{
    constexpr int RelatedSetCount = sizeof...( Sets );

    std::array<int, RelatedSetCount> SetCounts { };

    for ( const auto& EffectiveStat : EffectiveStatRanges )
    {
        CountSet<0, Sets...>( SetCounts, EffectiveStat.Set );
        CommonStats += EffectiveStat;
    }

    ApplyAllSetByCount<0, Sets...>( CommonStats, SetCounts );

    return CommonStats;
}

template <char ElementType>
inline EffectiveStats
CalculateCombinationalStat( auto&& EffectiveStatRanges, const EffectiveStats& CommonStats )
{
#define SWITCH_TYPE( type ) \
    if constexpr ( ElementType == e##type )

    SWITCH_TYPE( FireDamagePercentage )
    {
        return CountAndApplySets<eMoltenRift, eMoonlitClouds, eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( AirDamagePercentage )
    {
        return CountAndApplySets<eSierraGale, eMoonlitClouds, eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( IceDamagePercentage )
    {
        return CountAndApplySets<eFreezingFrost, eMoonlitClouds, eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( ElectricDamagePercentage )
    {
        return CountAndApplySets<eVoidThunder, eMoonlitClouds, eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( DarkDamagePercentage )
    {
        return CountAndApplySets<eSunSinkingEclipse, eMoonlitClouds, eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( LightDamagePercentage )
    {
        return CountAndApplySets<eCelestialLight, eMoonlitClouds, eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }

#undef SWITCH_TYPE

    std::unreachable( );
}
