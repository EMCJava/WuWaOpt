//
// Created by EMCJava on 8/5/2024.
//

#pragma once

enum class StatType : char {
    eAttack                     = '#',
    eAttackPercentage           = '&',
    eAutoAttackDamagePercentage = '-',
    eHeavyAttackPercentage      = '=',
    eUltDamagePercentage        = '*',
    eSkillDamagePercentage      = '+',
    eHealBonusPercentage        = 'h',
    eFireDamagePercentage       = 'f',
    eAirDamagePercentage        = 'a',
    eIceDamagePercentage        = 'i',
    eElectricDamagePercentage   = 'e',
    eDarkDamagePercentage       = 'd',
    eLightDamagePercentage      = 'l',
    eDefence                    = '(',
    eDefencePercentage          = ')',
    eHealth                     = '!',
    eHealthPercentage           = '~',
    eRegenPercentage            = '@',
    eCritDamage                 = '^',
    eCritRate                   = '%',
};