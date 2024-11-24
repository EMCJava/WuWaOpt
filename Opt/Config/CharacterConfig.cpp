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
CharacterConfig::GetBaseFoundation( ) const noexcept
{
    return GetBaseFoundation( CharacterStatsFoundation );
}
FloatTy
CharacterConfig::GetBaseFoundation( StatsFoundation TargetFoundation ) const noexcept
{
    switch ( CharacterStatsFoundation )
    {
    case StatsFoundation::eFoundationAttack: return std::ranges::fold_left( StatsCompositions, 0.f, []( const FloatTy Acc, const StatsComposition& Composition ) {
        return Composition.Enabled ? Acc + Composition.CompositionStats.flat_attack : Acc;
    } );
    case StatsFoundation::eFoundationHealth: return std::ranges::fold_left( StatsCompositions, 0.f, []( const FloatTy Acc, const StatsComposition& Composition ) {
        return Composition.Enabled ? Acc + Composition.CompositionStats.flat_health : Acc;
    } );
    case StatsFoundation::eFoundationDefence: return std::ranges::fold_left( StatsCompositions, 0.f, []( const FloatTy Acc, const StatsComposition& Composition ) {
        return Composition.Enabled ? Acc + Composition.CompositionStats.flat_defence : Acc;
    } );
    }
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
        StatsCompositions.emplace_back( true, std::string( Name ) );
        DesiredStat = std::prev( StatsCompositions.end( ) );
    }

    return DesiredStat->CompositionStats;
}

void
CharacterConfig::UpdateOverallStats( ) noexcept
{
    CharacterOverallStats =
        std::ranges::fold_left(
            StatsCompositions,
            EffectiveStats { },
            []( const auto Acc, const StatsComposition& Composition ) {
                return Composition.Enabled ? Acc + Composition.CompositionStats : Acc;
            } );

    CharacterOverallDeepenStats =
        std::ranges::fold_left(
            StatsCompositions,
            SkillMultiplierConfig { },
            []( const auto Acc, const StatsComposition& Composition ) {
                return Composition.Enabled ? Acc + Composition.CompositionDeepenStats : Acc;
            } );

    CharacterOverallDeepenStats /= 100;
}

EffectiveStats
CharacterConfig::GetCombinedStatsWithoutFlatAttack( ) const noexcept
{
    auto CommonStats        = CharacterOverallStats;
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
    CommonStats.heal_buff /= 100;

    return CommonStats;
}


YAML::Node
ToNode( const CharacterConfig& rhs ) noexcept
{
    YAML::Node Node;

    Node[ "Multiplier" ] = rhs.SkillConfig;
    Node[ "Element" ]    = std::string( magic_enum::enum_name( rhs.CharacterElement ) );
    Node[ "Foundation" ] = std::string( magic_enum::enum_name( rhs.CharacterStatsFoundation ) );

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
        for ( const auto& [ Enabled, CompositionName, CompositionStats, CompositionDeepenStats ] : rhs.StatsCompositions )
        {
            auto CompositionSlot        = CompositionNode[ CompositionName ];
            CompositionSlot             = CompositionStats;
            CompositionSlot[ "Deepen" ] = CompositionDeepenStats;
            if ( !Enabled )
            {
                CompositionSlot[ "Disables" ] = true;
            }
        }
    }

    if ( !rhs.EquippedEchoHashes.empty( ) )
    {
        auto EchoHashesNode = Node[ "EquippedEchoHashes" ];
        for ( auto EchoHash : rhs.EquippedEchoHashes )
        {
            EchoHashesNode.push_back( EchoHash );
        }
    }

    return Node;
}

bool
FromNode( const YAML::Node& Node, CharacterConfig& rhs ) noexcept
{
    if ( const auto Value = Node[ "Multiplier" ] ) rhs.SkillConfig = Value.as<SkillMultiplierConfig>( );
    if ( const auto Value = Node[ "Element" ] ) rhs.CharacterElement = magic_enum::enum_cast<ElementType>( Value.as<std::string>( ) ).value_or( ElementType::eFireElement );
    if ( const auto Value = Node[ "Foundation" ] ) rhs.CharacterStatsFoundation = magic_enum::enum_cast<StatsFoundation>( Value.as<std::string>( ) ).value_or( StatsFoundation::eFoundationAttack );
    if ( const auto Value = Node[ "CharacterLevel" ] ) rhs.CharacterLevel = Value.as<int>( );
    if ( const auto Value = Node[ "EnemyLevel" ] ) rhs.EnemyLevel = Value.as<int>( );
    if ( const auto Value = Node[ "EnemyElementResistance" ] ) rhs.ElementResistance = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "EnemyElementDamageReduce" ] ) rhs.ElementDamageReduce = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "Profile" ] ) rhs.CharacterProfilePath = Value.as<std::string>( );

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
        rhs.StatsCompositions =
            CompositionsNode
            | std::views::transform( []( const YAML::const_iterator ::value_type& CompositionNode ) {
                  const bool Disabled                  = CompositionNode.second[ "Disables" ].IsDefined( ) && CompositionNode.second[ "Disables" ].as<bool>( );
                  const auto SkillMultiplierConfigNode = CompositionNode.second[ "Deepen" ];
                  auto       a                         = Dump( YAML::Node( CompositionNode.second ) );
                  return StatsComposition { .Enabled          = !Disabled,
                                            .CompositionName  = CompositionNode.first.as<std::string>( ),
                                            .CompositionStats = CompositionNode.second.as<EffectiveStats>( ),
                                            .CompositionDeepenStats =
                                                SkillMultiplierConfigNode.IsDefined( )
                                                ? CompositionNode.second[ "Deepen" ].as<SkillMultiplierConfig>( )
                                                : SkillMultiplierConfig {} };
              } )
            | std::ranges::to<std::vector>( );
    }

    if ( rhs.StatsCompositions.empty( ) )
    {
        spdlog::warn( "Empty Stats composition node" );
        rhs.StatsCompositions.emplace_back( "#0" );
    }

    if ( auto EchoHashesNode = Node[ "EquippedEchoHashes" ] )
    {
        rhs.EquippedEchoHashes =
            EchoHashesNode
            | std::views::transform( []( const YAML::const_iterator ::value_type& CompositionNode ) {
                  return CompositionNode.as<size_t>( );
              } )
            | std::ranges::to<std::vector>( );
    }

    rhs.UpdateOverallStats( );

    return true;
}