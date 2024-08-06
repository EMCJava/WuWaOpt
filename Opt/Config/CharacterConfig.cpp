//
// Created by EMCJava on 8/5/2024.
//

#include "CharacterConfig.hpp"

#include <magic_enum.hpp>

FloatTy
CharacterConfig::GetResistances( ) const noexcept
{
    return ( (FloatTy) 100 + CharacterLevel ) / ( 199 + CharacterLevel + EnemyLevel ) * ( 1 - ElementResistance ) * ( 1 - ElementDamageReduce );
}

FloatTy
CharacterConfig::GetBaseAttack( ) const noexcept
{
    return WeaponStats.flat_attack + CharacterStats.flat_attack;
}

EffectiveStats
CharacterConfig::GetCombinedStats( ) const noexcept
{
    auto CommonStats        = WeaponStats + CharacterStats;
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

    Node[ "Weapon" ]     = rhs.WeaponStats;
    Node[ "Character" ]  = rhs.CharacterStats;
    Node[ "Multiplier" ] = rhs.SkillMultiplierConfig;
    Node[ "Element" ]    = std::string( magic_enum::enum_name( rhs.CharacterElement ) );

    Node[ "CharacterLevel" ]           = std::format( "{}", rhs.CharacterLevel );
    Node[ "EnemyLevel" ]               = std::format( "{}", rhs.EnemyLevel );
    Node[ "EnemyElementResistance" ]   = std::format( "{}", rhs.ElementResistance );
    Node[ "EnemyElementDamageReduce" ] = std::format( "{}", rhs.ElementDamageReduce );

    if ( !rhs.CharacterProfilePath.empty( ) )
    {
        Node[ "Profile" ] = rhs.CharacterProfilePath;
    }

    return Node;
}

bool
FromNode( const YAML::Node& Node, CharacterConfig& rhs ) noexcept
{
    rhs.WeaponStats           = Node[ "Weapon" ].as<EffectiveStats>( );
    rhs.CharacterStats        = Node[ "Character" ].as<EffectiveStats>( );
    rhs.SkillMultiplierConfig = Node[ "Multiplier" ].as<SkillMultiplierConfig>( );
    rhs.CharacterElement      = magic_enum::enum_cast<ElementType>( Node[ "Element" ].as<std::string>( ) ).value_or( ElementType::eFireElement );
    rhs.CharacterLevel        = Node[ "CharacterLevel" ].as<int>( );
    rhs.EnemyLevel            = Node[ "EnemyLevel" ].as<int>( );
    rhs.ElementResistance     = Node[ "EnemyElementResistance" ].as<FloatTy>( );
    rhs.ElementDamageReduce   = Node[ "EnemyElementDamageReduce" ].as<FloatTy>( );

    if ( Node[ "Profile" ] )
    {
        rhs.CharacterProfilePath = Node[ "Profile" ].as<std::string>( );
    }

    return true;
}