//
// Created by EMCJava on 7/25/2024.
//

#pragma once

#include <Common/Stat/EffectiveStats.hpp>

#include <Loca/Loca.hpp>

#include <yaml-cpp/yaml.h>

struct OptimizerConfig {
    Language LastUsedLanguage = Language::Undefined;

    // + 1 when saved to file
    int InternalStateID = 0;

    bool AskedCheckForNewVersion  = false;
    bool ShouldCheckForNewVersion = false;

    int OptimizerVersionCode = 0;

    void ReadConfig( );
    void SaveConfig( );
    void VersionUpgrade( );
};

YAML::Node ToNode( const OptimizerConfig& rhs ) noexcept;
bool       FromNode( const YAML::Node& Node, OptimizerConfig& rhs ) noexcept;

namespace YAML
{
template <>
struct convert<OptimizerConfig> {
    static Node encode( const OptimizerConfig& rhs )
    {
        return ToNode( rhs );
    }
    static bool decode( const Node& node, OptimizerConfig& rhs )
    {
        return FromNode( node, rhs );
    }
};
}   // namespace YAML