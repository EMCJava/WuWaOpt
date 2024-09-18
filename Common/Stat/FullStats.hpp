//
// Created by EMCJava on 8/5/2024.
//

#pragma once

#include "EffectiveStats.hpp"
#include "StatType.hpp"
#include "EchoSet.hpp"

#include <Common/ElementType.hpp>
#include <Common/Types.hpp>

#include <Loca/Loca.hpp>

#include <yaml-cpp/yaml.h>

#include <string>

struct FullStats {
    int         Cost  = 1;
    int         Level = 0;
    EchoSet     Set   = EchoSet::eEchoSetNone;
    std::string EchoName;
    int         NameID = -1;

    /*
     *
     * Percentage stats
     *
     * */
    FloatTy AutoAttackDamagePercentage = 0;
    FloatTy HeavyAttackPercentage      = 0;
    FloatTy UltDamagePercentage        = 0;
    FloatTy SkillDamagePercentage      = 0;

    FloatTy HealBonusPercentage      = 0;
    FloatTy FireDamagePercentage     = 0;
    FloatTy AirDamagePercentage      = 0;
    FloatTy IceDamagePercentage      = 0;
    FloatTy ElectricDamagePercentage = 0;
    FloatTy DarkDamagePercentage     = 0;
    FloatTy LightDamagePercentage    = 0;

    FloatTy AttackPercentage  = 0;
    FloatTy DefencePercentage = 0;
    FloatTy HealthPercentage  = 0;
    FloatTy RegenPercentage   = 0;

    /*
     *
     * Flat stats
     *
     * */
    FloatTy Attack  = 0;
    FloatTy Defence = 0;
    FloatTy Health  = 0;

    /*
     *
     * Special Stats
     *
     * */
    FloatTy CritDamage = 0;
    FloatTy CritRate   = 0;

    std::string Occupation;
    std::size_t EchoHash = 0;

    [[nodiscard]] std::string_view GetSetName( ) const noexcept;
    [[nodiscard]] std::string      BriefStat( const Loca& L ) const noexcept;
    [[nodiscard]] std::string      DetailStat( const Loca& L ) const noexcept;

    [[nodiscard]] std::size_t Hash( ) const noexcept;
};

template <ElementType ETy>
inline constexpr auto
GetElementBonusPtr( )
{
#define SWITCH_TYPE( type )                               \
    if constexpr ( ETy == ElementType::e##type##Element ) \
    {                                                     \
        return &FullStats::type##DamagePercentage;        \
    }

    // clang-format off
         SWITCH_TYPE( Fire )
    else SWITCH_TYPE( Ice )
    else SWITCH_TYPE( Air )
    else SWITCH_TYPE( Electric )
    else SWITCH_TYPE( Dark )
    else SWITCH_TYPE( Light )
    // clang-format on

#undef SWITCH_TYPE

        std::unreachable( );
}

template <ElementType ETy>
EffectiveStats
ToEffectiveStats( const FullStats& MatchResult )
{
    return EffectiveStats {
        .Set               = MatchResult.Set,
        .NameID            = MatchResult.NameID,
        .Cost              = MatchResult.Cost,
        .flat_attack       = MatchResult.Attack,
        .regen             = MatchResult.RegenPercentage,
        .percentage_attack = MatchResult.AttackPercentage,
        .buff_multiplier   = MatchResult.*GetElementBonusPtr<ETy>( ),
        .crit_rate         = MatchResult.CritRate,
        .crit_damage       = MatchResult.CritDamage,
        .auto_attack_buff  = MatchResult.AutoAttackDamagePercentage,
        .heavy_attack_buff = MatchResult.HeavyAttackPercentage,
        .skill_buff        = MatchResult.SkillDamagePercentage,
        .ult_buff          = MatchResult.UltDamagePercentage };
}

YAML::Node ToNode( const FullStats& rhs ) noexcept;
bool       FromNode( const YAML::Node& Node, FullStats& rhs ) noexcept;

namespace YAML
{
template <>
struct convert<FullStats> {
    static Node encode( const FullStats& rhs )
    {
        return ToNode( rhs );
    }
    static bool decode( const Node& node, FullStats& rhs )
    {
        return FromNode( node, rhs );
    }
};
}   // namespace YAML