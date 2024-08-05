//
// Created by EMCJava on 8/5/2024.
//

#include "CharacterConfig.hpp"

#include <magic_enum.hpp>

YAML::Node
ToNode( const CharacterConfig& rhs ) noexcept
{
    YAML::Node Node;

    Node[ "Weapon" ]     = rhs.m_WeaponStats;
    Node[ "Character" ]  = rhs.m_CharacterStats;
    Node[ "Multiplier" ] = rhs.m_SkillMultiplierConfig;
    Node[ "Element" ]    = std::string( magic_enum::enum_name( rhs.m_CharacterElement ) );

    Node[ "CharacterLevel" ]           = std::format( "{}", rhs.m_CharacterLevel );
    Node[ "EnemyLevel" ]               = std::format( "{}", rhs.m_EnemyLevel );
    Node[ "EnemyElementResistance" ]   = std::format( "{}", rhs.m_ElementResistance );
    Node[ "EnemyElementDamageReduce" ] = std::format( "{}", rhs.m_ElementDamageReduce );

    return Node;
}

bool
FromNode( const YAML::Node& Node, CharacterConfig& rhs ) noexcept
{
    rhs.m_WeaponStats           = Node[ "Weapon" ].as<EffectiveStats>( );
    rhs.m_CharacterStats        = Node[ "Character" ].as<EffectiveStats>( );
    rhs.m_SkillMultiplierConfig = Node[ "Multiplier" ].as<SkillMultiplierConfig>( );
    rhs.m_CharacterElement      = magic_enum::enum_cast<ElementType>( Node[ "Element" ].as<std::string>( ) ).value_or( ElementType::eFireElement );
    rhs.m_CharacterLevel        = Node[ "CharacterLevel" ].as<int>( );
    rhs.m_EnemyLevel            = Node[ "EnemyLevel" ].as<int>( );
    rhs.m_ElementResistance     = Node[ "EnemyElementResistance" ].as<FloatTy>( );
    rhs.m_ElementDamageReduce   = Node[ "EnemyElementDamageReduce" ].as<FloatTy>( );

    return true;
}