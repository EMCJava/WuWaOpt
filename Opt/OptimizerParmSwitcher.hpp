//
// Created by EMCJava on 7/17/2024.
//

#pragma once

#include <Common/Stat/StatType.hpp>
#include <Common/ElementType.hpp>

#include "SetStat.hpp"
#include "WuWaGa.hpp"

#include <array>

namespace OptimizerParmSwitcher
{
extern std::array<decltype( &::CalculateCombinationalStat<ElementType::eFireElement> ), /* 6 Elements */ 6> CalculateCombinationalStatPtrs;
extern std::array<decltype( &WuWaGA::Run<ElementType::eFireElement> ), /* 6 Elements */ 6>                  RunPtrs;

inline EffectiveStats
SwitchCalculateCombinationalStat( int ElementOffset, auto&& Ranges, auto&& EffectiveStats )
{
    return ( CalculateCombinationalStatPtrs[ ElementOffset ] )( Ranges, EffectiveStats );
}

inline void
SwitchRun( WuWaGA& GA, ElementType Element, auto&& Ranges, auto&& EffectiveStats, const SkillMultiplierConfig* OptimizeMultiplierConfig, const auto& Constraints )
{
    return ( GA.*RunPtrs[ (int) Element ] )( Ranges, EffectiveStats, OptimizeMultiplierConfig, Constraints );
}
};   // namespace OptimizerParmSwitcher
