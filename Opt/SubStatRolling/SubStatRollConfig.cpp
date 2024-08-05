//
// Created by EMCJava on 7/25/2024.
//

#include "SubStatRollConfig.hpp"

#include <format>
#include <ranges>

void
SubStatRollConfig::SetValueStrings( bool IsPercentage )
{
    std::ranges::copy(
        Values
            | std::views::transform( [ IsPercentage ]( const ValueRollRate& Roll ) {
                if ( IsPercentage )
                {
                    return std::format( "{:.1f}", Roll.Value * 100 ) + "%";

                } else
                {
                    return std::format( "{:.0f}", Roll.Value );
                }
            } ),
        ValueStrings.begin( ) );
}

const char*
SubStatRollConfig::GetValueString( void* user_data, int idx )
{
    const auto This = static_cast<SubStatRollConfig*>( user_data );
    return This->ValueStrings[ idx ].c_str( );
}
