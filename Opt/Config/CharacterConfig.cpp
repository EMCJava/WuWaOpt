//
// Created by EMCJava on 8/5/2024.
//

#include "CharacterConfig.hpp"

#include <magic_enum.hpp>

#include <spdlog/spdlog.h>

#include <numeric>
#include <ranges>

FloatTy
CharacterConfig::GetResistances( ) const noexcept
{
    return ( (FloatTy) 100 + CharacterLevel ) / ( 199 + CharacterLevel + EnemyLevel ) * ( 1 - ElementResistance ) * ( 1 - ElementDamageReduce );
}

FloatTy
CharacterConfig::GetBaseAttack( ) const noexcept
{
    return std::ranges::fold_left( StatsCompositions, 0.f, []( const FloatTy Acc, const StatsComposition& Composition ) {
        return Acc + Composition.CompositionStats.flat_attack;
    } );
}

EffectiveStats&
CharacterConfig::GetStatsComposition( const std::string& Name )
{
    auto DesiredStat =
        std::ranges::find_if( StatsCompositions,
                              [ &Name ]( const StatsComposition& Composition ) {
                                  return Composition.CompositionName == Name;
                              } );

    if ( DesiredStat == StatsCompositions.end( ) )
    {
        StatsCompositions.emplace_back( std::string( Name ) );
        DesiredStat = std::prev( StatsCompositions.end( ) );
    }

    return DesiredStat->CompositionStats;
}

EffectiveStats
CharacterConfig::GetCombinedStats( ) const noexcept
{
    auto CommonStats =
        std::ranges::fold_left(
            StatsCompositions,
            EffectiveStats { },
            []( const auto Acc, const StatsComposition& Composition ) {
                return Acc + Composition.CompositionStats;
            } );
    CommonStats.flat_attack = 0;

    CommonStats.crit_damage /= 100;
    CommonStats.crit_rate /= 100;
    CommonStats.percentage_attack /= 100;
    CommonStats.buff_multiplier /= 100;
    CommonStats.regen /= 100;
    CommonStats.auto_attack_buff /= 100;
    CommonStats.heavy_attack_buff /= 100;
    CommonStats.skill_buff /= 100;
    CommonStats.ult_buff /= 100;

    return CommonStats;
}


YAML::Node
ToNode( const CharacterConfig& rhs ) noexcept
{
    YAML::Node Node;

    Node[ "Multiplier" ] = rhs.SkillConfig;
    Node[ "Deepen" ]     = rhs.DeepenConfig;
    Node[ "Element" ]    = std::string( magic_enum::enum_name( rhs.CharacterElement ) );

    Node[ "CharacterLevel" ]           = std::format( "{}", rhs.CharacterLevel );
    Node[ "EnemyLevel" ]               = std::format( "{}", rhs.EnemyLevel );
    Node[ "EnemyElementResistance" ]   = std::format( "{}", rhs.ElementResistance );
    Node[ "EnemyElementDamageReduce" ] = std::format( "{}", rhs.ElementDamageReduce );

    if ( !rhs.CharacterProfilePath.empty( ) )
    {
        Node[ "Profile" ] = rhs.CharacterProfilePath;
    }

    if ( !rhs.StatsCompositions.empty( ) )
    {
        auto CompositionNode = Node[ "StatsCompositions" ];
        for ( const auto& [ CompositionName, CompositionStats ] : rhs.StatsCompositions )
        {
            CompositionNode[ CompositionName ] = CompositionStats;
        }
    }

    return Node;
}

bool
FromNode( const YAML::Node& Node, CharacterConfig& rhs ) noexcept
{
    rhs.SkillConfig = Node[ "Multiplier" ].as<SkillMultiplierConfig>( );
    if ( const auto Value = Node[ "Deepen" ] ) rhs.DeepenConfig = Value.as<SkillMultiplierConfig>( );
    rhs.CharacterElement    = magic_enum::enum_cast<ElementType>( Node[ "Element" ].as<std::string>( ) ).value_or( ElementType::eFireElement );
    rhs.CharacterLevel      = Node[ "CharacterLevel" ].as<int>( );
    rhs.EnemyLevel          = Node[ "EnemyLevel" ].as<int>( );
    rhs.ElementResistance   = Node[ "EnemyElementResistance" ].as<FloatTy>( );
    rhs.ElementDamageReduce = Node[ "EnemyElementDamageReduce" ].as<FloatTy>( );

    if ( Node[ "Profile" ] )
    {
        rhs.CharacterProfilePath = Node[ "Profile" ].as<std::string>( );
    }

    // Old version config
    const auto CharacterNode = Node[ "Character" ];
    const auto WeaponNode    = Node[ "Weapon" ];

    auto CompositionsNode = Node[ "StatsCompositions" ];
    if ( Node[ "Character" ] || Node[ "Weapon" ] )
    {
        // Old config version
        if ( CompositionsNode )
        {
            // New config data in old version???

            spdlog::warn( "Overriding the CompositionsNode with explicit Weapon / Character stats" );
        } else
        {
            std::construct_at( std::addressof( CompositionsNode ) );
        }

        if ( CharacterNode ) CompositionsNode[ "Character" ] = CharacterNode;
        if ( WeaponNode ) CompositionsNode[ "Weapon" ] = WeaponNode;
    }

    if ( CompositionsNode )
    {
        spdlog::trace( "{}", YAML::Dump( CompositionsNode ) );

        rhs.StatsCompositions =
            CompositionsNode
            | std::views::transform( []( const YAML::const_iterator ::value_type& CompositionNode ) {
                  return StatsComposition { .CompositionName  = CompositionNode.first.as<std::string>( ),
                                            .CompositionStats = CompositionNode.second.as<EffectiveStats>( ) };
              } )
            | std::ranges::to<std::vector>( );
    }

    return true;
}