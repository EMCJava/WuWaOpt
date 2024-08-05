//
// Created by EMCJava on 8/5/2024.
//

#pragma once

#include <Opt/FullStats.hpp>

class CharacterConfig
{
    EffectiveStats m_WeaponStats { };
    EffectiveStats m_CharacterStats { };

    MultiplierConfig m_SkillMultiplierConfig { };
    ElementType      m_CharacterElement { };

    int     m_CharacterLevel { };
    int     m_EnemyLevel { };
    FloatTy m_ElementResistance { };
    FloatTy m_ElementDamageReduce { };
};
