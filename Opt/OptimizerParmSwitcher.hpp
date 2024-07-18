//
// Created by EMCJava on 7/17/2024.
//

#pragma once

#include "FullStats.hpp"
#include "WuWaGa.hpp"

#include <array>

namespace OptimizerParmSwitcher
{
extern std::array<decltype( &::CalculateCombinationalStat<eFireDamagePercentage> ), /* 6 Elements */ 6> CalculateCombinationalStatPtrs;
extern std::array<decltype( &WuWaGA::Run<eFireDamagePercentage> ), /* 6 Elements */ 6>                  RunPtrs;

inline EffectiveStats
SwitchCalculateCombinationalStat( int ElementOffset, auto&& Ranges, auto&& EffectiveStats )
{
    return ( CalculateCombinationalStatPtrs[ ElementOffset ] )( Ranges, EffectiveStats );
}

inline void
SwitchRun( WuWaGA& GA, int ElementOffset, auto&& Ranges, auto&& EffectiveStats, const MultiplierConfig* OptimizeMultiplierConfig )
{
    return ( GA.*RunPtrs[ ElementOffset ] )( Ranges, EffectiveStats, OptimizeMultiplierConfig );
}
};   // namespace OptimizerParmSwitcher
