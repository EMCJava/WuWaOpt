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

std::array<std::array<decltype( &WuWaGA::Run<eFireDamagePercentage, eAutoAttackDamagePercentage> ), /* 4 Damage Type */ 4>, /* 6 Elements */ 6> OptimizerParmSwitcher::RunPtrs {
    std::array<decltype( &WuWaGA::Run<eFireDamagePercentage, eAutoAttackDamagePercentage> ), /* 4 Damage Type */ 4> {    &WuWaGA::Run<eFireDamagePercentage, eAutoAttackDamagePercentage>,     &WuWaGA::Run<eFireDamagePercentage, eHeavyAttackPercentage>,     &WuWaGA::Run<eFireDamagePercentage, eUltDamagePercentage>,     &WuWaGA::Run<eFireDamagePercentage, eSkillDamagePercentage>},
    std::array<decltype( &WuWaGA::Run<eFireDamagePercentage, eAutoAttackDamagePercentage> ), /* 4 Damage Type */ 4> {     &WuWaGA::Run<eAirDamagePercentage, eAutoAttackDamagePercentage>,      &WuWaGA::Run<eAirDamagePercentage, eHeavyAttackPercentage>,      &WuWaGA::Run<eAirDamagePercentage, eUltDamagePercentage>,      &WuWaGA::Run<eAirDamagePercentage, eSkillDamagePercentage>},
    std::array<decltype( &WuWaGA::Run<eFireDamagePercentage, eAutoAttackDamagePercentage> ), /* 4 Damage Type */ 4> {     &WuWaGA::Run<eIceDamagePercentage, eAutoAttackDamagePercentage>,      &WuWaGA::Run<eIceDamagePercentage, eHeavyAttackPercentage>,      &WuWaGA::Run<eIceDamagePercentage, eUltDamagePercentage>,      &WuWaGA::Run<eIceDamagePercentage, eSkillDamagePercentage>},
    std::array<decltype( &WuWaGA::Run<eFireDamagePercentage, eAutoAttackDamagePercentage> ), /* 4 Damage Type */ 4> {&WuWaGA::Run<eElectricDamagePercentage, eAutoAttackDamagePercentage>, &WuWaGA::Run<eElectricDamagePercentage, eHeavyAttackPercentage>, &WuWaGA::Run<eElectricDamagePercentage, eUltDamagePercentage>, &WuWaGA::Run<eElectricDamagePercentage, eSkillDamagePercentage>},
    std::array<decltype( &WuWaGA::Run<eFireDamagePercentage, eAutoAttackDamagePercentage> ), /* 4 Damage Type */ 4> {    &WuWaGA::Run<eDarkDamagePercentage, eAutoAttackDamagePercentage>,     &WuWaGA::Run<eDarkDamagePercentage, eHeavyAttackPercentage>,     &WuWaGA::Run<eDarkDamagePercentage, eUltDamagePercentage>,     &WuWaGA::Run<eDarkDamagePercentage, eSkillDamagePercentage>},
    std::array<decltype( &WuWaGA::Run<eFireDamagePercentage, eAutoAttackDamagePercentage> ), /* 4 Damage Type */ 4> {   &WuWaGA::Run<eLightDamagePercentage, eAutoAttackDamagePercentage>,    &WuWaGA::Run<eLightDamagePercentage, eHeavyAttackPercentage>,    &WuWaGA::Run<eLightDamagePercentage, eUltDamagePercentage>,    &WuWaGA::Run<eLightDamagePercentage, eSkillDamagePercentage>},
};
