//
// Created by EMCJava on 7/25/2024.
//

#include "OptimizerConfig.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>

void
OptimizerConfig::ReadConfig( )
{
    std::ifstream ConfigFile { "OptimizerConfig.yaml" };

    if ( ConfigFile )
    {
        *this = YAML::Load( ConfigFile ).as<OptimizerConfig>( );
    }

    VersionUpgrade( );
}

void
OptimizerConfig::SaveConfig( )
{
    InternalStateID++;

    std::ofstream ConfigFile( "OptimizerConfig.yaml" );
    if ( ConfigFile.is_open( ) )
    {
        YAML::Node ResultYAMLEchos { *this };
        ConfigFile << ResultYAMLEchos;
        ConfigFile.close( );
    } else
    {
        spdlog::error( "Unable to open OptimizerConfig.yaml for writing" );
    }
}

void
OptimizerConfig::VersionUpgrade( )
{
    while ( true )
    {
        if ( OptimizerVersionCode == 0 )
        {
            OptimizerVersionCode = 1;
            continue;
        }

        break;
    }

    SaveConfig( );
}

YAML::Node
ToNode( const OptimizerConfig& rhs ) noexcept
{
    YAML::Node Node;

    Node[ "LastUsedLanguage" ]         = rhs.LastUsedLanguage;
    Node[ "InternalStateID" ]          = rhs.InternalStateID;
    Node[ "AskedCheckForNewVersion" ]  = rhs.AskedCheckForNewVersion;
    Node[ "ShouldCheckForNewVersion" ] = rhs.ShouldCheckForNewVersion;
    Node[ "OptimizerVersionCode" ]     = rhs.OptimizerVersionCode;

    return Node;
}

bool
FromNode( const YAML::Node& Node, OptimizerConfig& rhs ) noexcept
{

    if ( const auto Value = Node[ "LastUsedLanguage" ]; Value ) rhs.LastUsedLanguage = Value.as<Language>( );
    if ( const auto Value = Node[ "InternalStateID" ]; Value ) rhs.InternalStateID = Value.as<int>( );
    if ( const auto Value = Node[ "AskedCheckForNewVersion" ]; Value ) rhs.AskedCheckForNewVersion = Value.as<bool>( );
    if ( const auto Value = Node[ "ShouldCheckForNewVersion" ]; Value ) rhs.ShouldCheckForNewVersion = Value.as<bool>( );
    if ( const auto Value = Node[ "OptimizerVersionCode" ]; Value ) rhs.OptimizerVersionCode = Value.as<int>( );

    return true;
}