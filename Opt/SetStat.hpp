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
template <EchoSet Set>
inline void
ApplyTwoSetEffect( EffectiveStats& Stats )
{
    Stats.buff_multiplier += 0.1f;
}

template <>
inline void
ApplyTwoSetEffect<EchoSet::eLingeringTunes>( EffectiveStats& Stats )
{
    Stats.percentage_attack += 0.1f;
}

template <>
inline void
ApplyTwoSetEffect<EchoSet::eMoonlitClouds>( EffectiveStats& Stats )
{
    Stats.regen += 0.1f;
}

template <>
inline void
ApplyTwoSetEffect<EchoSet::eRejuvenatingGlow>( EffectiveStats& Stats )
{
    Stats.heal_buff += 0.1f;
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
ApplyFiveSetEffect<EchoSet::eRejuvenatingGlow>( EffectiveStats& Stats )
{
    /* When performing the Outro Skill, the ATK of the entire team's Resonator increases by 15%, lasting 30 seconds. */
}

template <>
inline void
ApplyFiveSetEffect<EchoSet::eMoonlitClouds>( EffectiveStats& Stats )
{
    /* After using an Outro Skill, the ATK of the next Resonator to enter the field increases by 22.5%, lasting 15 seconds. */
}

template <>
inline void
ApplyFiveSetEffect<EchoSet::eLingeringTunes>( EffectiveStats& Stats )
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

    std::array<int, RelatedSetCount>               SetCounts { 0 };
    std::array<SetNameOccupation, RelatedSetCount> OccupationMask { 0 };

    for ( const auto& EffectiveStat : EffectiveStatRanges )
    {
        CountSet<0, Sets...>( SetCounts, OccupationMask, EffectiveStat.Set, EffectiveStat.NameID );
        CommonStats += EffectiveStat;
    }

    ApplyAllSetByCount<0, Sets...>( CommonStats, SetCounts );

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
        return CountAndApplySets<EchoSet::eMoltenRift, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( Air )
    {
        return CountAndApplySets<EchoSet::eSierraGale, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( Ice )
    {
        return CountAndApplySets<EchoSet::eFreezingFrost, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( Electric )
    {
        return CountAndApplySets<EchoSet::eVoidThunder, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( Dark )
    {
        return CountAndApplySets<EchoSet::eSunSinkingEclipse, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }
    else SWITCH_TYPE( Light )
    {
        return CountAndApplySets<EchoSet::eCelestialLight, EchoSet::eMoonlitClouds, EchoSet::eLingeringTunes>( EffectiveStatRanges, CommonStats );
    }

#undef SWITCH_TYPE

    std::unreachable( );
}
