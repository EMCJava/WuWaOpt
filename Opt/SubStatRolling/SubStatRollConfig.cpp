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

const std::vector<SubStatRollConfig> FullSubStatRollConfigs = []( ) {
    std::vector<SubStatRollConfig> Result;
    SubStatRollConfig              TemplateConfig;

    // for ( int i = 0; i < 5 /* Non-effective rolls */; ++i )
    //     SubStatRollConfigs.push_back( TemplateConfig );

    TemplateConfig.IsEffective   = true;
    TemplateConfig.PossibleRolls = 8;
    TemplateConfig.Values =
        {
            ValueRollRate {0.064, 0.0739},
            ValueRollRate {0.071,  0.069},
            ValueRollRate {0.079, 0.2072},
            ValueRollRate {0.086,  0.249},
            ValueRollRate {0.094, 0.1823},
            ValueRollRate {0.101,  0.136},
            ValueRollRate {0.109, 0.0534},
            ValueRollRate {0.116, 0.0293}
    };
    TemplateConfig.SetValueStrings( );

    TemplateConfig.ValuePtr = &EffectiveStats::percentage_attack;
    Result.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::ult_buff;
    Result.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::heavy_attack_buff;
    Result.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::skill_buff;
    Result.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::auto_attack_buff;
    Result.push_back( TemplateConfig );

    TemplateConfig.ValuePtr = &EffectiveStats::crit_rate;
    std::ranges::for_each( std::views::zip(
                               std::vector<FloatTy> { 0.063, 0.069, 0.075, 0.081, 0.087, 0.093, 0.099, 0.105 },
                               TemplateConfig.Values ),
                           []( std::pair<FloatTy, ValueRollRate&> Pair ) {
                               Pair.second.Value = Pair.first;
                           } );
    TemplateConfig.SetValueStrings( );
    Result.push_back( TemplateConfig );

    TemplateConfig.ValuePtr = &EffectiveStats::crit_damage;
    std::ranges::for_each( std::views::zip(
                               std::vector<FloatTy> { 0.126, 0.138, 0.15, 0.162, 0.174, 0.186, 0.198, 0.21 },
                               TemplateConfig.Values ),
                           []( std::pair<FloatTy, ValueRollRate&> Pair ) {
                               Pair.second.Value = Pair.first;
                           } );
    TemplateConfig.SetValueStrings( );
    Result.push_back( TemplateConfig );

    memset( &TemplateConfig.Values, 0, sizeof( TemplateConfig.Values ) );
    TemplateConfig.ValuePtr      = &EffectiveStats::flat_attack;
    TemplateConfig.PossibleRolls = 4;
    TemplateConfig.Values =
        {
            ValueRollRate {30, 0.1243},
            ValueRollRate {40, 0.4621},
            ValueRollRate {50, 0.3857},
            ValueRollRate {60, 0.0279}
    };
    TemplateConfig.SetValueStrings( false );
    Result.push_back( TemplateConfig );

    return Result;
}( );

const std::vector<SubStatRollConfig> MaxSubStatRollConfigs = []( ) {
    std::vector<SubStatRollConfig> Result;
    SubStatRollConfig              TemplateConfig;

    TemplateConfig.IsEffective   = true;
    TemplateConfig.PossibleRolls = 1;
    TemplateConfig.Values        = {
        ValueRollRate { 0.116, 1 }
    };
    TemplateConfig.SetValueStrings( );

    TemplateConfig.ValuePtr = &EffectiveStats::percentage_attack;
    Result.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::ult_buff;
    Result.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::heavy_attack_buff;
    Result.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::skill_buff;
    Result.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::auto_attack_buff;
    Result.push_back( TemplateConfig );

    TemplateConfig.ValuePtr = &EffectiveStats::crit_rate;
    TemplateConfig.Values   = {
        ValueRollRate { 0.105, 1 }
    };
    TemplateConfig.SetValueStrings( );
    Result.push_back( TemplateConfig );

    TemplateConfig.ValuePtr = &EffectiveStats::crit_damage;
    TemplateConfig.Values   = {
        ValueRollRate { 0.21, 1 }
    };
    TemplateConfig.SetValueStrings( );
    Result.push_back( TemplateConfig );

    TemplateConfig.ValuePtr = &EffectiveStats::flat_attack;
    TemplateConfig.Values   = {
        ValueRollRate { 60, 1 }
    };
    TemplateConfig.SetValueStrings( false );
    Result.push_back( TemplateConfig );

    return Result;
}( );