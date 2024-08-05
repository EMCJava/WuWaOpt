//
// Created by EMCJava on 8/5/2024.
//

#pragma once

#include <Common/Stat/EffectiveStats.hpp>

#include <yaml-cpp/yaml.h>

struct CharacterConfig {
    EffectiveStats m_WeaponStats { };
    EffectiveStats m_CharacterStats { };

    SkillMultiplierConfig m_SkillMultiplierConfig { };
    ElementType           m_CharacterElement { };

    int     m_CharacterLevel { };
    int     m_EnemyLevel { };
    FloatTy m_ElementResistance { };
    FloatTy m_ElementDamageReduce { };
};

YAML::Node ToNode( const CharacterConfig& rhs ) noexcept;
bool       FromNode( const YAML::Node& Node, CharacterConfig& rhs ) noexcept;

namespace YAML
{
template <>
struct convert<CharacterConfig> {
    static Node encode( const CharacterConfig& rhs )
    {
        return ToNode( rhs );
    }
    static bool decode( const Node& node, CharacterConfig& rhs )
    {
        return FromNode( node, rhs );
    }
};
}   // namespace YAML