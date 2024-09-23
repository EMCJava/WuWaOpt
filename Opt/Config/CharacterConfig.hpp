//
// Created by EMCJava on 8/5/2024.
//

#pragma once

#include <Common/Stat/EffectiveStats.hpp>

#include <yaml-cpp/yaml.h>

struct StatsComposition {
    std::string    CompositionName;
    EffectiveStats CompositionStats { };
};

struct CharacterConfig {
    EffectiveStats                CharacterOverallStats { };
    std::vector<StatsComposition> StatsCompositions { };

    SkillMultiplierConfig SkillConfig { };
    SkillMultiplierConfig DeepenConfig { };
    ElementType           CharacterElement { };

    int     CharacterLevel { };
    int     EnemyLevel { };
    FloatTy ElementResistance { };
    FloatTy ElementDamageReduce { };

    std::string CharacterProfilePath { };

    // Incremental stage ID to identify different stats
    int InternalStageID = 0;

    [[nodiscard]] FloatTy GetResistances( ) const noexcept;
    [[nodiscard]] FloatTy GetBaseAttack( ) const noexcept;

    EffectiveStats GetCombinedStats( ) const noexcept;
    [[nodiscard]] EffectiveStats& GetStatsComposition( const std::string& Name );

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