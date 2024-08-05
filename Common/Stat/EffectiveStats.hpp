//
// Created by EMCJava on 8/5/2024.
//

#pragma once

#include <Opt/Config/SkillMultiplierConfig.hpp>

#include <Common/Types.hpp>
#include <Common/ElementType.hpp>

#include "EchoSet.hpp"

#include <yaml-cpp/yaml.h>

struct StatValueConfig;
struct EffectiveStats {

    EchoSet Set               = EchoSet::eEchoSetNone;
    int     NameID            = 0;
    int     Cost              = 0;
    FloatTy flat_attack       = 0;
    FloatTy regen             = 0;
    FloatTy percentage_attack = 0;
    FloatTy buff_multiplier   = 0;
    FloatTy crit_rate         = 0;
    FloatTy crit_damage       = 0;

    FloatTy auto_attack_buff  = 0;
    FloatTy heavy_attack_buff = 0;
    FloatTy skill_buff        = 0;
    FloatTy ult_buff          = 0;

    EffectiveStats&              operator+=( const EffectiveStats& Other ) noexcept;
    [[nodiscard]] EffectiveStats operator+( const EffectiveStats& Other ) const noexcept;
    [[nodiscard]] EffectiveStats operator+( const StatValueConfig& StatValue ) const noexcept;
    [[nodiscard]] EffectiveStats operator-( const EffectiveStats& Other ) const noexcept;
    [[nodiscard]] bool           operator==( const EffectiveStats& Other ) const noexcept;

    static constexpr FloatTy CharacterDefaultRegen      = 1;
    static constexpr FloatTy CharacterDefaultCritRate   = 0.05;
    static constexpr FloatTy CharacterDefaultCritDamage = 1.5;

    [[nodiscard]] FloatTy RegenStat( ) const noexcept;
    [[nodiscard]] FloatTy CritRateStat( ) const noexcept;
    [[nodiscard]] FloatTy CritDamageStat( ) const noexcept;

    [[nodiscard]] FloatTy AttackStat( FloatTy base_attack ) const noexcept;
    [[nodiscard]] FloatTy NormalDamage( FloatTy base_attack, const SkillMultiplierConfig* multiplier_config ) const noexcept;
    [[nodiscard]] FloatTy CritDamage( FloatTy base_attack, const SkillMultiplierConfig* multiplier_config ) const noexcept;
    [[nodiscard]] FloatTy ExpectedDamage( FloatTy base_attack, const SkillMultiplierConfig* multiplier_config ) const noexcept;
    void                  ExpectedDamage( FloatTy base_attack, const SkillMultiplierConfig* multiplier_config,
                                          FloatTy& ND, FloatTy& CD, FloatTy& ED ) const noexcept;

    std::string_view   GetSetName( ) const noexcept;
    static const char* GetStatName( const FloatTy EffectiveStats::*stat_type );
};

struct StatValueConfig {
    FloatTy EffectiveStats::*ValuePtr = { };
    FloatTy                  Value    = { };
};

YAML::Node ToNode( const EffectiveStats& rhs ) noexcept;
bool       FromNode( const YAML::Node& Node, EffectiveStats& rhs ) noexcept;

namespace YAML
{
template <>
struct convert<EffectiveStats> {
    static Node encode( const EffectiveStats& rhs )
    {
        return ToNode( rhs );
    }
    static bool decode( const Node& node, EffectiveStats& rhs )
    {
        return FromNode( node, rhs );
    }
};
}   // namespace YAML