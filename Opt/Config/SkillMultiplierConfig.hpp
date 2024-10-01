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

    SkillMultiplierConfig operator+( const SkillMultiplierConfig& Other ) const noexcept
    {
        return {
            .auto_attack_multiplier  = auto_attack_multiplier + Other.auto_attack_multiplier,
            .heavy_attack_multiplier = heavy_attack_multiplier + Other.heavy_attack_multiplier,
            .skill_multiplier        = skill_multiplier + Other.skill_multiplier,
            .ult_multiplier          = ult_multiplier + Other.ult_multiplier,
        };
    }

    SkillMultiplierConfig& operator/=( auto Scaler ) noexcept
    {
        auto_attack_multiplier /= Scaler;
        heavy_attack_multiplier /= Scaler;
        skill_multiplier /= Scaler;
        ult_multiplier /= Scaler;

        return *this;
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
        if ( const auto Value = Node[ "AutoAttack" ]; Value ) rhs.auto_attack_multiplier = Value.as<FloatTy>( );
        if ( const auto Value = Node[ "HeavyAttack" ]; Value ) rhs.heavy_attack_multiplier = Value.as<FloatTy>( );
        if ( const auto Value = Node[ "Skill" ]; Value ) rhs.skill_multiplier = Value.as<FloatTy>( );
        if ( const auto Value = Node[ "Ultimate" ]; Value ) rhs.ult_multiplier = Value.as<FloatTy>( );

        return true;
    }
};
}   // namespace YAML