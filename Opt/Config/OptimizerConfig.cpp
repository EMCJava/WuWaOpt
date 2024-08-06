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
    static_assert( std::is_trivially_copy_assignable_v<OptimizerConfig> );
    std::ifstream OptimizerConfigFile( "oc.data", std::ios::binary );
    if ( OptimizerConfigFile )
    {
        OptimizerConfigFile.ignore( std::numeric_limits<std::streamsize>::max( ) );
        const auto FileLength = OptimizerConfigFile.gcount( );
        OptimizerConfigFile.clear( );   //  Since ignore will have set eof.
        OptimizerConfigFile.seekg( 0, std::ios_base::beg );

        auto ActualReadSize = sizeof( OptimizerConfig );
        if ( FileLength < ActualReadSize )
        {
            ActualReadSize = FileLength;
            spdlog::warn( "Data file version mismatch, reading all available data." );
        } else if ( FileLength > ActualReadSize )
        {
            OptimizerConfigFile.close( );
            std::filesystem::remove( "oc.data" );
            spdlog::warn( "Data file version mismatch, discarding all data." );
            return;
        }

        OptimizerConfigFile.read( (char*) this, ActualReadSize );
    }
}

void
OptimizerConfig::SaveConfig( )
{
    InternalStateID++;

    static_assert( std::is_trivially_copy_assignable_v<OptimizerConfig> );
    std::ofstream OptimizerConfigFile( "oc.data", std::ios::binary );
    if ( OptimizerConfigFile )
    {
        OptimizerConfigFile.write( (char*) this, sizeof( OptimizerConfig ) );
    }
}