//
// Created by EMCJava on 6/2/2024.
//

#pragma once

#include <cstdint>
#include <format>
#include <array>

#include "SetStat.hpp"

struct EffectiveStats {

    EchoSet Set               = eFreezingFrost;
    int     Cost              = 0;
    FloatTy flat_attack       = 0;
    FloatTy regen             = 0;
    FloatTy percentage_attack = 0;
    FloatTy buff_multiplier   = 0;
    FloatTy crit_rate         = 0;
    FloatTy crit_damage       = 0;

    EffectiveStats& operator+=( const EffectiveStats& Other ) noexcept
    {
        Set = eEchoSetNone;

        Cost += Other.Cost;
        flat_attack += Other.flat_attack;
        regen += Other.regen;
        percentage_attack += Other.percentage_attack;
        buff_multiplier += Other.buff_multiplier;
        crit_rate += Other.crit_rate;
        crit_damage += Other.crit_damage;

        return *this;
    }

    EffectiveStats operator+( const EffectiveStats& Other ) const noexcept
    {
        return EffectiveStats {
            eEchoSetNone,
            Cost + Other.Cost,
            flat_attack + Other.flat_attack,
            regen + Other.regen,
            percentage_attack + Other.percentage_attack,
            buff_multiplier + Other.buff_multiplier,
            crit_rate + Other.crit_rate,
            crit_damage + Other.crit_damage };
    }

    EffectiveStats operator-( const EffectiveStats& Other ) const noexcept
    {
        return EffectiveStats {
            eEchoSetNone,
            Cost - Other.Cost,
            flat_attack - Other.flat_attack,
            regen - Other.regen,
            percentage_attack - Other.percentage_attack,
            buff_multiplier - Other.buff_multiplier,
            crit_rate - Other.crit_rate,
            crit_damage - Other.crit_damage };
    }

    bool operator==( const EffectiveStats& Other ) const noexcept
    {
#define CLOSE( x, y ) ( std::abs( x - y ) < 0.0001f )

        if ( Set != Other.Set ) return false;
        if ( Cost != Other.Cost ) return false;
        if ( !CLOSE( flat_attack, Other.flat_attack ) ) return false;
        if ( !CLOSE( regen, Other.regen ) ) return false;
        if ( !CLOSE( percentage_attack, Other.percentage_attack ) ) return false;
        if ( !CLOSE( buff_multiplier, Other.buff_multiplier ) ) return false;
        if ( !CLOSE( crit_rate, Other.crit_rate ) ) return false;
        if ( !CLOSE( crit_damage, Other.crit_damage ) ) return false;

#undef CLOSE

        return true;
    }

    FloatTy NormalDamage( auto&& base_attack ) const noexcept
    {
        const FloatTy attack = base_attack * ( 1 + percentage_attack ) + flat_attack;
        return attack * ( 1 + buff_multiplier );
    }

    FloatTy CritDamage( auto&& base_attack ) const noexcept
    {
        return NormalDamage( base_attack ) * crit_damage;
    }

    FloatTy ExpectedDamage( auto&& base_attack ) const noexcept
    {
        return NormalDamage( base_attack ) * ( 1 + std::min( crit_rate, (FloatTy) 1 ) * crit_damage );
    }
};

enum StatType : char {
    eAttack                     = '#',
    eAttackPercentage           = '&',
    eAutoAttackDamagePercentage = '-',
    eHeavyAttackPercentage      = '=',
    eUltDamagePercentage        = '*',
    eSkillDamagePercentage      = '+',
    eHealBonusPercentage        = 'h',
    eFireDamagePercentage       = 'f',
    eAirDamagePercentage        = 'a',
    eIceDamagePercentage        = 'i',
    eElectricDamagePercentage   = 'e',
    eDarkDamagePercentage       = 'd',
    eLightDamagePercentage      = 'l',
    eDefence                    = '(',
    eDefencePercentage          = ')',
    eHealth                     = '!',
    eHealthPercentage           = '~',
    eRegenPercentage            = '@',
    eCritDamage                 = '^',
    eCritRate                   = '%',
};

struct FullStats {
    int     Cost  = 1;
    int     Level = 0;
    EchoSet Set   = eEchoSetNone;

    /*
     *
     * Percentage stats
     *
     * */
    FloatTy AutoAttackDamagePercentage = 0;
    FloatTy HeavyAttackPercentage      = 0;
    FloatTy UltDamagePercentage        = 0;
    FloatTy SkillDamagePercentage      = 0;

    FloatTy HealBonusPercentage      = 0;
    FloatTy FireDamagePercentage     = 0;
    FloatTy AirDamagePercentage      = 0;
    FloatTy IceDamagePercentage      = 0;
    FloatTy ElectricDamagePercentage = 0;
    FloatTy DarkDamagePercentage     = 0;
    FloatTy LightDamagePercentage    = 0;

    FloatTy AttackPercentage  = 0;
    FloatTy DefencePercentage = 0;
    FloatTy HealthPercentage  = 0;
    FloatTy RegenPercentage   = 0;

    /*
     *
     * Flat stats
     *
     * */
    FloatTy Attack  = 0;
    FloatTy Defence = 0;
    FloatTy Health  = 0;

    /*
     *
     * Special Stats
     *
     * */
    FloatTy CritDamage = 0;
    FloatTy CritRate   = 0;

    [[nodiscard]] std::string GetSetName( ) const noexcept
    {
        return json( Set ).get<std::string>( );
    }

    [[nodiscard]] std::string BriefStat( ) const noexcept
    {
        return std::format( "Cost: {} Level: {:2}",
                            Cost,
                            Level );
    }

    [[nodiscard]] std::string DetailStat( ) const noexcept
    {
        std::stringstream ss;

        int StatIndex    = 0;
        int SubStatIndex = 0;

        if ( std::abs( AutoAttackDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "AutoAttackDamagePercentage", AutoAttackDamagePercentage * 100 );
        if ( std::abs( HeavyAttackPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "HeavyAttackPercentage", HeavyAttackPercentage * 100 );
        if ( std::abs( UltDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "UltDamagePercentage", UltDamagePercentage * 100 );
        if ( std::abs( SkillDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "SkillDamagePercentage", SkillDamagePercentage * 100 );
        if ( std::abs( HealBonusPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "HealBonusPercentage", HealBonusPercentage * 100 );
        if ( std::abs( FireDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "FireDamagePercentage", FireDamagePercentage * 100 );
        if ( std::abs( AirDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "AirDamagePercentage", AirDamagePercentage * 100 );
        if ( std::abs( IceDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "IceDamagePercentage", IceDamagePercentage * 100 );
        if ( std::abs( ElectricDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "ElectricDamagePercentage", ElectricDamagePercentage * 100 );
        if ( std::abs( DarkDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "DarkDamagePercentage", DarkDamagePercentage * 100 );
        if ( std::abs( LightDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "LightDamagePercentage", LightDamagePercentage * 100 );
        if ( std::abs( AttackPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "AttackPercentage", AttackPercentage * 100 );
        if ( std::abs( DefencePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "DefencePercentage", DefencePercentage * 100 );
        if ( std::abs( HealthPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "HealthPercentage", HealthPercentage * 100 );
        if ( std::abs( RegenPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "RegenPercentage", RegenPercentage * 100 );
        if ( std::abs( Attack ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "Flat Attack", Attack );
        if ( std::abs( Defence ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "Flat Defence", Defence );
        if ( std::abs( Health ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "Flat Health", Health );
        if ( std::abs( CritDamage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "CritDamage", CritDamage * 100 );
        if ( std::abs( CritRate ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:10.1f}\n", ++StatIndex, "CritRate", CritRate * 100 );

        return ss.str( );
    }
};


#define NLOHMANN_JSON_TO_LAZY( v1 ) \
    if ( !std::is_floating_point_v<std::decay_t<decltype( nlohmann_json_t.v1 )>> || std::abs( nlohmann_json_t.v1 ) > 0.0001f ) nlohmann_json_j[ #v1 ] = nlohmann_json_t.v1;
#define NLOHMANN_JSON_FROM_LAZY( v1 ) \
    if ( nlohmann_json_j.contains( #v1 ) ) nlohmann_json_j.at( #v1 ).get_to( nlohmann_json_t.v1 );

#define NLOHMANN_LAZY_TYPE( Type, ... )                                                     \
    inline void to_json( nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t )     \
    {                                                                                       \
        NLOHMANN_JSON_EXPAND( NLOHMANN_JSON_PASTE( NLOHMANN_JSON_TO_LAZY, __VA_ARGS__ ) )   \
    }                                                                                       \
    inline void from_json( const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t )   \
    {                                                                                       \
        NLOHMANN_JSON_EXPAND( NLOHMANN_JSON_PASTE( NLOHMANN_JSON_FROM_LAZY, __VA_ARGS__ ) ) \
    }


NLOHMANN_LAZY_TYPE( FullStats,
                    Cost,
                    Level,
                    Set,
                    AutoAttackDamagePercentage,
                    HeavyAttackPercentage,
                    UltDamagePercentage,
                    SkillDamagePercentage,
                    HealBonusPercentage,
                    FireDamagePercentage,
                    AirDamagePercentage,
                    IceDamagePercentage,
                    ElectricDamagePercentage,
                    DarkDamagePercentage,
                    LightDamagePercentage,
                    AttackPercentage,
                    DefencePercentage,
                    HealthPercentage,
                    RegenPercentage,
                    Attack,
                    Defence,
                    Health,
                    CritDamage,
                    CritRate )

template <char ElementType>
constexpr auto
GetElementBonusPtr( )
{
#define SWITCH_TYPE( type )                 \
    if constexpr ( ElementType == e##type ) \
    {                                       \
        return &FullStats::type;            \
    }

    // clang-format off
    SWITCH_TYPE( FireDamagePercentage )
    else SWITCH_TYPE( IceDamagePercentage )
    else SWITCH_TYPE( AirDamagePercentage )
    else SWITCH_TYPE( ElectricDamagePercentage )
    else SWITCH_TYPE( DarkDamagePercentage )
    else SWITCH_TYPE( LightDamagePercentage )
    // clang-format on

#undef SWITCH_TYPE

        std::unreachable( );
}

template <char DamageType>
constexpr auto
GetSkillBonusPtr( )
{
    if constexpr ( DamageType == eAutoAttackDamagePercentage )
    {
        return &FullStats::AutoAttackDamagePercentage;
    } else if constexpr ( DamageType == eSkillDamagePercentage )
    {
        return &FullStats::SkillDamagePercentage;
    } else if constexpr ( DamageType == eUltDamagePercentage )
    {
        return &FullStats::UltDamagePercentage;
    }

    std::unreachable( );
}

template <char ElementType, char DamageType>
EffectiveStats
ToEffectiveStats( const FullStats& MatchResult )
{
    return EffectiveStats {
        .Set               = MatchResult.Set,
        .Cost              = MatchResult.Cost,
        .flat_attack       = MatchResult.Attack,
        .regen             = MatchResult.RegenPercentage,
        .percentage_attack = MatchResult.AttackPercentage,
        .buff_multiplier =
            MatchResult.*GetElementBonusPtr<ElementType>( )
            + MatchResult.*GetSkillBonusPtr<DamageType>( ),
        .crit_rate   = MatchResult.CritRate,
        .crit_damage = MatchResult.CritDamage };
}
