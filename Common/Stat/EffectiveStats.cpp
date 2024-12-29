//
// Created by EMCJava on 8/5/2024.
//

#include "EffectiveStats.hpp"

#include <magic_enum/magic_enum.hpp>

EffectiveStats
EffectiveStats::operator+( const StatValueConfig& StatValue ) const noexcept
{
    EffectiveStats Result = *this;
    Result.*( StatValue.ValuePtr ) += StatValue.Value;
    return Result;
}

EffectiveStats&
EffectiveStats::operator+=( const EffectiveStats& Other ) noexcept
{
    Set    = EchoSet::eEchoSetNone;
    NameID = -1;

    Cost += Other.Cost;

    flat_attack += Other.flat_attack;
    percentage_attack += Other.percentage_attack;

    flat_health += Other.flat_health;
    percentage_health += Other.percentage_health;

    flat_defence += Other.flat_defence;
    percentage_defence += Other.percentage_defence;

    regen += Other.regen;
    buff_multiplier += Other.buff_multiplier;
    crit_rate += Other.crit_rate;
    crit_damage += Other.crit_damage;

    auto_attack_buff += Other.auto_attack_buff;
    heavy_attack_buff += Other.heavy_attack_buff;
    skill_buff += Other.skill_buff;
    ult_buff += Other.ult_buff;
    heal_buff += Other.heal_buff;

    return *this;
}

EffectiveStats
EffectiveStats::operator+( const EffectiveStats& Other ) const noexcept
{
    return EffectiveStats {
        .Set                = EchoSet::eEchoSetNone,
        .NameID             = -1,
        .Cost               = Cost + Other.Cost,
        .flat_attack        = flat_attack + Other.flat_attack,
        .percentage_attack  = percentage_attack + Other.percentage_attack,
        .flat_health        = flat_health + Other.flat_health,
        .percentage_health  = percentage_health + Other.percentage_health,
        .flat_defence       = flat_defence + Other.flat_defence,
        .percentage_defence = percentage_defence + Other.percentage_defence,
        .regen              = regen + Other.regen,
        .buff_multiplier    = buff_multiplier + Other.buff_multiplier,
        .crit_rate          = crit_rate + Other.crit_rate,
        .crit_damage        = crit_damage + Other.crit_damage,
        .auto_attack_buff   = auto_attack_buff + Other.auto_attack_buff,
        .heavy_attack_buff  = heavy_attack_buff + Other.heavy_attack_buff,
        .skill_buff         = skill_buff + Other.skill_buff,
        .ult_buff           = ult_buff + Other.ult_buff,
        .heal_buff          = heal_buff + Other.heal_buff };
}

EffectiveStats
EffectiveStats::operator-( const EffectiveStats& Other ) const noexcept
{
    return EffectiveStats {
        .Set                = EchoSet::eEchoSetNone,
        .NameID             = -1,
        .Cost               = Cost - Other.Cost,
        .flat_attack        = flat_attack - Other.flat_attack,
        .percentage_attack  = percentage_attack - Other.percentage_attack,
        .flat_health        = flat_health - Other.flat_health,
        .percentage_health  = percentage_health - Other.percentage_health,
        .flat_defence       = flat_defence - Other.flat_defence,
        .percentage_defence = percentage_defence - Other.percentage_defence,
        .regen              = regen - Other.regen,
        .buff_multiplier    = buff_multiplier - Other.buff_multiplier,
        .crit_rate          = crit_rate - Other.crit_rate,
        .crit_damage        = crit_damage - Other.crit_damage,
        .auto_attack_buff   = auto_attack_buff - Other.auto_attack_buff,
        .heavy_attack_buff  = heavy_attack_buff - Other.heavy_attack_buff,
        .skill_buff         = skill_buff - Other.skill_buff,
        .ult_buff           = ult_buff - Other.ult_buff,
        .heal_buff          = heal_buff - Other.heal_buff };
}

bool
EffectiveStats::operator==( const EffectiveStats& Other ) const noexcept
{
#define CLOSE( x, y ) ( std::abs( x - y ) < 0.0001f )

    if ( Set != Other.Set ) return false;
    if ( NameID != Other.NameID ) return false;
    if ( Cost != Other.Cost ) return false;
    if ( !CLOSE( flat_attack, Other.flat_attack ) ) return false;
    if ( !CLOSE( percentage_attack, Other.percentage_attack ) ) return false;
    if ( !CLOSE( flat_health, Other.flat_health ) ) return false;
    if ( !CLOSE( percentage_health, Other.percentage_health ) ) return false;
    if ( !CLOSE( flat_defence, Other.flat_defence ) ) return false;
    if ( !CLOSE( percentage_defence, Other.percentage_defence ) ) return false;
    if ( !CLOSE( regen, Other.regen ) ) return false;
    if ( !CLOSE( buff_multiplier, Other.buff_multiplier ) ) return false;
    if ( !CLOSE( crit_rate, Other.crit_rate ) ) return false;
    if ( !CLOSE( crit_damage, Other.crit_damage ) ) return false;
    if ( !CLOSE( auto_attack_buff, Other.auto_attack_buff ) ) return false;
    if ( !CLOSE( heavy_attack_buff, Other.heavy_attack_buff ) ) return false;
    if ( !CLOSE( skill_buff, Other.skill_buff ) ) return false;
    if ( !CLOSE( ult_buff, Other.ult_buff ) ) return false;
    if ( !CLOSE( heal_buff, Other.heal_buff ) ) return false;

#undef CLOSE

    return true;
}

FloatTy
EffectiveStats::RegenStat( ) const noexcept
{
    return regen + CharacterDefaultRegen;
}

FloatTy
EffectiveStats::CritRateStat( ) const noexcept
{
    return crit_rate + CharacterDefaultCritRate;
}

FloatTy
EffectiveStats::CritDamageStat( ) const noexcept
{
    return crit_damage + CharacterDefaultCritDamage;
}

FloatTy
EffectiveStats::FoundationStat( StatsFoundation character_foundation, FloatTy foundation_base ) const noexcept
{
    switch ( character_foundation )
    {
    case StatsFoundation::eFoundationAttack: return foundation_base * ( 1 + percentage_attack ) + flat_attack;
    case StatsFoundation::eFoundationHealth: return foundation_base * ( 1 + percentage_health ) + flat_health;
    case StatsFoundation::eFoundationDefence: return foundation_base * ( 1 + percentage_defence ) + flat_defence;
    }

    __assume( false );
}
FloatTy
EffectiveStats::HealingAmount( StatsFoundation character_foundation, FloatTy foundation_base, const SkillMultiplierConfig* multiplier_config, const SkillMultiplierConfig* deepen_config ) const noexcept
{
    return multiplier_config->heal_multiplier * FoundationStat( character_foundation, foundation_base ) * ( 1.f + heal_buff );
}

FloatTy
EffectiveStats::NormalDamage( StatsFoundation character_foundation, FloatTy foundation_base, const SkillMultiplierConfig* multiplier_config, const SkillMultiplierConfig* deepen_config ) const noexcept
{
    const auto FinalFoundationStat = FoundationStat( character_foundation, foundation_base );

    // clang-format off
    return multiplier_config->auto_attack_multiplier  * (1 + deepen_config->auto_attack_multiplier ) * FinalFoundationStat * ( 1.f + buff_multiplier + auto_attack_buff  )
         + multiplier_config->heavy_attack_multiplier * (1 + deepen_config->heavy_attack_multiplier) * FinalFoundationStat * ( 1.f + buff_multiplier + heavy_attack_buff )
         + multiplier_config->skill_multiplier        * (1 + deepen_config->skill_multiplier       ) * FinalFoundationStat * ( 1.f + buff_multiplier + skill_buff        )
         + multiplier_config->ult_multiplier          * (1 + deepen_config->ult_multiplier         ) * FinalFoundationStat * ( 1.f + buff_multiplier + ult_buff          );
    // clang-format on
}

FloatTy
EffectiveStats::CritDamage( StatsFoundation character_foundation, FloatTy foundation_base, const SkillMultiplierConfig* multiplier_config, const SkillMultiplierConfig* deepen_config ) const noexcept
{
    return NormalDamage( character_foundation, foundation_base, multiplier_config, deepen_config ) * CritDamageStat( );
}

FloatTy
EffectiveStats::ExpectedDamage( StatsFoundation character_foundation, FloatTy foundation_base, const SkillMultiplierConfig* multiplier_config, const SkillMultiplierConfig* deepen_config ) const noexcept
{
    return NormalDamage( character_foundation, foundation_base, multiplier_config, deepen_config ) * ( 1 + std::min( CritRateStat( ), (FloatTy) 1 ) * ( 0.5f + crit_damage ) );
}
FloatTy
EffectiveStats::OptimizingValue( StatsFoundation character_foundation, FloatTy foundation_base, FloatTy OptimizingDamagePercentage, const SkillMultiplierConfig* multiplier_config, const SkillMultiplierConfig* deepen_config ) const noexcept
{
    return ExpectedDamage( character_foundation, foundation_base, multiplier_config, deepen_config ) * OptimizingDamagePercentage + HealingAmount( character_foundation, foundation_base, multiplier_config, deepen_config );
}

void
EffectiveStats::ExtractOptimizingStats( StatsFoundation character_foundation, FloatTy foundation_base, FloatTy OptimizingDamagePercentage, const SkillMultiplierConfig* multiplier_config, const SkillMultiplierConfig* deepen_config, FloatTy& HA, FloatTy& ND, FloatTy& CD, FloatTy& ED, FloatTy& OV ) const noexcept
{
    HA = HealingAmount( character_foundation, foundation_base, multiplier_config, deepen_config );
    ND = NormalDamage( character_foundation, foundation_base, multiplier_config, deepen_config ) * OptimizingDamagePercentage;
    CD = ND * CritDamageStat( );
    ED = ND * ( 1 + std::min( CritRateStat( ), (FloatTy) 1 ) * ( 0.5f + crit_damage ) );
    OV = HA + ED;
}

const char*
EffectiveStats::GetStatName( const FloatTy EffectiveStats::* stat_type )
{
#define PtrSwitch( x, y )                  \
    if ( stat_type == &EffectiveStats::x ) \
        return y;                          \
    else

    PtrSwitch( flat_attack, "FlatAttack" )
        PtrSwitch( percentage_attack, "Attack%" )
            PtrSwitch( flat_health, "FlatHealth" )
                PtrSwitch( percentage_health, "Health%" )
                    PtrSwitch( flat_defence, "FlatDefence" )
                        PtrSwitch( percentage_defence, "Defence%" )
                            PtrSwitch( regen, "Regen%" )
                                PtrSwitch( buff_multiplier, "ElementBuff%" )
                                    PtrSwitch( crit_rate, "CritRate" )
                                        PtrSwitch( crit_damage, "CritDamage" )
                                            PtrSwitch( auto_attack_buff, "AutoAttack%" )
                                                PtrSwitch( heavy_attack_buff, "HeavyAttack%" )
                                                    PtrSwitch( skill_buff, "SkillDamage%" )
                                                        PtrSwitch( ult_buff, "UltDamage%" )
                                                            PtrSwitch( heal_buff, "Heal%" ) return "NonEffective";

#undef PtrSwitch
}

std::string_view
EffectiveStats::GetSetName( ) const noexcept
{
    return magic_enum::enum_name( Set );
}

YAML::Node
ToNode( const EffectiveStats& rhs ) noexcept
{
    YAML::Node Node;

    if ( rhs.Set != EchoSet::eEchoSetNone ) Node[ "Set" ] = std::string( rhs.GetSetName( ) );
    if ( rhs.Cost != 0 ) Node[ "Cost" ] = rhs.Cost;

    if ( std::abs( rhs.flat_attack ) > 0.001 ) Node[ "flat_attack" ] = std::format( "{}", rhs.flat_attack );
    if ( std::abs( rhs.percentage_attack ) > 0.001 ) Node[ "percentage_attack" ] = std::format( "{}", rhs.percentage_attack );
    if ( std::abs( rhs.flat_health ) > 0.001 ) Node[ "flat_health" ] = std::format( "{}", rhs.flat_health );
    if ( std::abs( rhs.percentage_health ) > 0.001 ) Node[ "percentage_health" ] = std::format( "{}", rhs.percentage_health );
    if ( std::abs( rhs.flat_defence ) > 0.001 ) Node[ "flat_defence" ] = std::format( "{}", rhs.flat_defence );
    if ( std::abs( rhs.percentage_defence ) > 0.001 ) Node[ "percentage_defence" ] = std::format( "{}", rhs.percentage_defence );

    Node[ "regen" ]             = std::format( "{}", rhs.regen );
    Node[ "buff_multiplier" ]   = std::format( "{}", rhs.buff_multiplier );
    Node[ "crit_rate" ]         = std::format( "{}", rhs.crit_rate );
    Node[ "crit_damage" ]       = std::format( "{}", rhs.crit_damage );
    Node[ "auto_attack_buff" ]  = std::format( "{}", rhs.auto_attack_buff );
    Node[ "heavy_attack_buff" ] = std::format( "{}", rhs.heavy_attack_buff );
    Node[ "skill_buff" ]        = std::format( "{}", rhs.skill_buff );
    Node[ "ult_buff" ]          = std::format( "{}", rhs.ult_buff );
    Node[ "heal_buff" ]         = std::format( "{}", rhs.heal_buff );

    return Node;
}

bool
FromNode( const YAML::Node& Node, EffectiveStats& rhs ) noexcept
{
    if ( Node[ "Set" ] )
        rhs.Set = magic_enum::enum_cast<EchoSet>( Node[ "Set" ].as<std::string>( ) ).value_or( EchoSet::eEchoSetNone );
    else
        rhs.Set = EchoSet::eEchoSetNone;   // Reset to default if "Set" field is missing

    if ( Node[ "Cost" ] )
        rhs.Cost = Node[ "Cost" ].as<int>( );
    else
        rhs.Cost = 0;   // Reset to default if "Cost" field is missing

    if ( const auto Value = Node[ "flat_attack" ]; Value ) rhs.flat_attack = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "percentage_attack" ]; Value ) rhs.percentage_attack = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "flat_health" ]; Value ) rhs.flat_health = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "percentage_health" ]; Value ) rhs.percentage_health = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "flat_defence" ]; Value ) rhs.flat_defence = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "percentage_defence" ]; Value ) rhs.percentage_defence = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "regen" ]; Value ) rhs.regen = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "buff_multiplier" ]; Value ) rhs.buff_multiplier = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "crit_rate" ]; Value ) rhs.crit_rate = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "crit_damage" ]; Value ) rhs.crit_damage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "auto_attack_buff" ]; Value ) rhs.auto_attack_buff = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "heavy_attack_buff" ]; Value ) rhs.heavy_attack_buff = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "skill_buff" ]; Value ) rhs.skill_buff = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "ult_buff" ]; Value ) rhs.ult_buff = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "heal_buff" ]; Value ) rhs.heal_buff = Value.as<FloatTy>( );

    return true;
}