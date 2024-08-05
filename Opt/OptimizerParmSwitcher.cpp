//
// Created by EMCJava on 7/17/2024.
//

#include "OptimizerParmSwitcher.hpp"

std::array<decltype( &::CalculateCombinationalStat<ElementType::eFireElement> ), /* 6 Elements */ 6> OptimizerParmSwitcher::CalculateCombinationalStatPtrs {
    &::CalculateCombinationalStat<ElementType::eFireElement>,
    &::CalculateCombinationalStat<ElementType::eAirElement>,
    &::CalculateCombinationalStat<ElementType::eIceElement>,
    &::CalculateCombinationalStat<ElementType::eElectricElement>,
    &::CalculateCombinationalStat<ElementType::eDarkElement>,
    &::CalculateCombinationalStat<ElementType::eLightElement>,
};

std::array<decltype( &WuWaGA::Run<ElementType::eFireElement> ), /* 6 Elements */ 6> OptimizerParmSwitcher::RunPtrs {
    &WuWaGA::Run<ElementType::eFireElement>,
    &WuWaGA::Run<ElementType::eAirElement>,
    &WuWaGA::Run<ElementType::eIceElement>,
    &WuWaGA::Run<ElementType::eElectricElement>,
    &WuWaGA::Run<ElementType::eDarkElement>,
    &WuWaGA::Run<ElementType::eLightElement>,
};
