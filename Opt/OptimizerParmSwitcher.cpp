//
// Created by EMCJava on 7/17/2024.
//

#include "OptimizerParmSwitcher.hpp"

std::array<decltype( &::CalculateCombinationalStat<eFireDamagePercentage> ), /* 6 Elements */ 6> OptimizerParmSwitcher::CalculateCombinationalStatPtrs {
    &::CalculateCombinationalStat<eFireDamagePercentage>,
    &::CalculateCombinationalStat<eAirDamagePercentage>,
    &::CalculateCombinationalStat<eIceDamagePercentage>,
    &::CalculateCombinationalStat<eElectricDamagePercentage>,
    &::CalculateCombinationalStat<eDarkDamagePercentage>,
    &::CalculateCombinationalStat<eLightDamagePercentage>,
};

std::array<decltype( &WuWaGA::Run<eFireDamagePercentage> ), /* 6 Elements */ 6> OptimizerParmSwitcher::RunPtrs {
    &WuWaGA::Run<eFireDamagePercentage>,
    &WuWaGA::Run<eAirDamagePercentage>,
    &WuWaGA::Run<eIceDamagePercentage>,
    &WuWaGA::Run<eElectricDamagePercentage>,
    &WuWaGA::Run<eDarkDamagePercentage>,
    &WuWaGA::Run<eLightDamagePercentage>,
};
