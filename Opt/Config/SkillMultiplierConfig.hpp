//
// Created by EMCJava on 8/5/2024.
//

#pragma once

#include <Common/Types.hpp>

#include <yaml-cpp/yaml.h>

struct SkillMultiplierConfig {

    FloatTy auto_attack_multiplier  = 0;
    FloatTy heavy_attack_multiplier = 0;
    FloatTy skill_multiplier        = 0;
    FloatTy ult_multiplier          = 0;

    bool operator==( const SkillMultiplierConfig& Other ) const noexcept
    {
        return auto_attack_multiplier == Other.auto_attack_multiplier && heavy_attack_multiplier == Other.heavy_attack_multiplier && skill_multiplier == Other.skill_multiplier && ult_multiplier == Other.ult_multiplier;
    }
};

namespace YAML
{
template <>
struct convert<SkillMultiplierConfig> {
    static Node encode( const SkillMultiplierConfig& rhs )
    {
        YAML::Node Node;

        Node[ "AutoAttack" ]  = std::format( "{}", rhs.auto_attack_multiplier );
        Node[ "HeavyAttack" ] = std::format( "{}", rhs.heavy_attack_multiplier );
        Node[ "Skill" ]       = std::format( "{}", rhs.skill_multiplier );
        Node[ "Ultimate" ]    = std::format( "{}", rhs.ult_multiplier );

        return Node;
    }
    static bool decode( const Node& Node, SkillMultiplierConfig& rhs )
    {
        rhs.auto_attack_multiplier  = Node[ "AutoAttack" ].as<FloatTy>( );
        rhs.heavy_attack_multiplier = Node[ "HeavyAttack" ].as<FloatTy>( );
        rhs.skill_multiplier        = Node[ "Skill" ].as<FloatTy>( );
        rhs.ult_multiplier          = Node[ "Ultimate" ].as<FloatTy>( );

        return true;
    }
};
}   // namespace YAML