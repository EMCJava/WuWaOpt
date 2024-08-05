//
// Created by EMCJava on 8/5/2024.
//

#include "EffectiveStats.hpp"


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
    regen += Other.regen;
    percentage_attack += Other.percentage_attack;
    buff_multiplier += Other.buff_multiplier;
    crit_rate += Other.crit_rate;
    crit_damage += Other.crit_damage;

    auto_attack_buff += Other.auto_attack_buff;
    heavy_attack_buff += Other.heavy_attack_buff;
    skill_buff += Other.skill_buff;
    ult_buff += Other.ult_buff;

    return *this;
}

EffectiveStats
EffectiveStats::operator+( const EffectiveStats& Other ) const noexcept
{
    return EffectiveStats {
        EchoSet::eEchoSetNone,
        -1,
        Cost + Other.Cost,
        flat_attack + Other.flat_attack,
        regen + Other.regen,
        percentage_attack + Other.percentage_attack,
        buff_multiplier + Other.buff_multiplier,
        crit_rate + Other.crit_rate,
        crit_damage + Other.crit_damage,
        auto_attack_buff + Other.auto_attack_buff,
        heavy_attack_buff + Other.heavy_attack_buff,
        skill_buff + Other.skill_buff,
        ult_buff + Other.ult_buff };
}

EffectiveStats
EffectiveStats::operator-( const EffectiveStats& Other ) const noexcept
{
    return EffectiveStats {
        EchoSet::eEchoSetNone,
        -1,
        Cost - Other.Cost,
        flat_attack - Other.flat_attack,
        regen - Other.regen,
        percentage_attack - Other.percentage_attack,
        buff_multiplier - Other.buff_multiplier,
        crit_rate - Other.crit_rate,
        crit_damage - Other.crit_damage,
        auto_attack_buff - Other.auto_attack_buff,
        heavy_attack_buff - Other.heavy_attack_buff,
        skill_buff - Other.skill_buff,
        ult_buff - Other.ult_buff };
}

bool
EffectiveStats::operator==( const EffectiveStats& Other ) const noexcept
{
#define CLOSE( x, y ) ( std::abs( x - y ) < 0.0001f )

    if ( Set != Other.Set ) return false;
    if ( NameID != Other.NameID ) return false;
    if ( Cost != Other.Cost ) return false;
    if ( !CLOSE( flat_attack, Other.flat_attack ) ) return false;
    if ( !CLOSE( regen, Other.regen ) ) return false;
    if ( !CLOSE( percentage_attack, Other.percentage_attack ) ) return false;
    if ( !CLOSE( buff_multiplier, Other.buff_multiplier ) ) return false;
    if ( !CLOSE( crit_rate, Other.crit_rate ) ) return false;
    if ( !CLOSE( crit_damage, Other.crit_damage ) ) return false;
    if ( !CLOSE( auto_attack_buff, Other.auto_attack_buff ) ) return false;
    if ( !CLOSE( heavy_attack_buff, Other.heavy_attack_buff ) ) return false;
    if ( !CLOSE( skill_buff, Other.skill_buff ) ) return false;
    if ( !CLOSE( ult_buff, Other.ult_buff ) ) return false;

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
EffectiveStats::AttackStat( FloatTy base_attack ) const noexcept
{
    return base_attack * ( 1 + percentage_attack ) + flat_attack;
}

FloatTy
EffectiveStats::NormalDamage( FloatTy base_attack, const SkillMultiplierConfig* multiplier_config ) const noexcept
{
    // clang-format off
    return multiplier_config->auto_attack_multiplier * AttackStat( base_attack ) * ( 1.f + buff_multiplier + auto_attack_buff  )
        + multiplier_config->heavy_attack_multiplier * AttackStat( base_attack ) * ( 1.f + buff_multiplier + heavy_attack_buff )
        + multiplier_config->skill_multiplier        * AttackStat( base_attack ) * ( 1.f + buff_multiplier + skill_buff        )
        + multiplier_config->ult_multiplier          * AttackStat( base_attack ) * ( 1.f + buff_multiplier + ult_buff          );
    // clang-format on
}

FloatTy
EffectiveStats::CritDamage( FloatTy base_attack, const SkillMultiplierConfig* multiplier_config ) const noexcept
{
    return NormalDamage( base_attack, multiplier_config ) * CritDamageStat( );
}

FloatTy
EffectiveStats::ExpectedDamage( FloatTy base_attack, const SkillMultiplierConfig* multiplier_config ) const noexcept
{
    return NormalDamage( base_attack, multiplier_config ) * ( 1 + std::min( CritRateStat( ), (FloatTy) 1 ) * ( 0.5f + crit_damage ) );
}

void
EffectiveStats::ExpectedDamage( FloatTy base_attack, const SkillMultiplierConfig* multiplier_config, FloatTy& ND, FloatTy& CD, FloatTy& ED ) const noexcept
{
    ND = NormalDamage( base_attack, multiplier_config );
    CD = ND * CritDamageStat( );
    ED = ND * ( 1 + std::min( CritRateStat( ), (FloatTy) 1 ) * ( 0.5f + crit_damage ) );
}

const char*
EffectiveStats::GetStatName( const FloatTy EffectiveStats::*stat_type )
{
#define PtrSwitch( x, y )                  \
    if ( stat_type == &EffectiveStats::x ) \
        return y;                          \
    else

    PtrSwitch( flat_attack, "FlatAttack" )
        PtrSwitch( regen, "Regen%" )
            PtrSwitch( percentage_attack, "Attack%" )
                PtrSwitch( buff_multiplier, "ElementBuff%" )
                    PtrSwitch( crit_rate, "CritRate" )
                        PtrSwitch( crit_damage, "CritDamage" )
                            PtrSwitch( auto_attack_buff, "AutoAttack%" )
                                PtrSwitch( heavy_attack_buff, "HeavyAttack%" )
                                    PtrSwitch( skill_buff, "SkillDamage%" )
                                        PtrSwitch( ult_buff, "UltDamage%" ) return "NonEffective";

#undef PtrSwitch
}