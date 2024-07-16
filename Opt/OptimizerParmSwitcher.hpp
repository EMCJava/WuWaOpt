//
// Created by EMCJava on 7/17/2024.
//

#pragma once

#include "FullStats.hpp"
#include "WuWaGa.hpp"

#include <array>

namespace OptimizerParmSwitcher
{
extern std::array<decltype( &::CalculateCombinationalStat<eFireDamagePercentage> ), /* 6 Elements */ 6>                                                CalculateCombinationalStatPtrs;
extern std::array<std::array<decltype( &WuWaGA::Run<eFireDamagePercentage, eAutoAttackDamagePercentage> ), /* 4 Damage Type */ 4>, /* 6 Elements */ 6> RunPtrs;

inline EffectiveStats
SwitchCalculateCombinationalStat( int ElementOffset, auto&& Ranges, auto&& EffectiveStats )
{
    return CalculateCombinationalStatPtrs[ ElementOffset ]( Ranges, EffectiveStats );
}

inline void
SwitchRun( WuWaGA& GA, int ElementOffset, int DamageTypeOffset, auto&& Ranges, auto&& EffectiveStats )
{
    return ( GA.*RunPtrs[ ElementOffset ][ DamageTypeOffset ] )( Ranges, EffectiveStats );
}
};   // namespace OptimizerParmSwitcher
