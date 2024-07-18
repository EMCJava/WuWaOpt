//
// Created by EMCJava on 6/2/2024.
//

#pragma once

#include <cstdint>
#include <format>
#include <array>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "OptUtil.hpp"

enum EchoSet : uint8_t {
    eFreezingFrost,
    eMoltenRift,
    eVoidThunder,
    eSierraGale,
    eCelestialLight,
    eSunSinkingEclipse,
    eRejuvenatingGlow,
    eMoonlitClouds,
    eLingeringTunes,
    eEchoSetCount,
    eEchoSetNone = eEchoSetCount
};

NLOHMANN_JSON_SERIALIZE_ENUM( EchoSet, {
                                           {    eFreezingFrost,     "eFreezingFrost"},
                                           {       eMoltenRift,        "eMoltenRift"},
                                           {      eVoidThunder,       "eVoidThunder"},
                                           {       eSierraGale,        "eSierraGale"},
                                           {   eCelestialLight,    "eCelestialLight"},
                                           {eSunSinkingEclipse, "eSunSinkingEclipse"},
                                           { eRejuvenatingGlow,  "eRejuvenatingGlow"},
                                           {    eMoonlitClouds,     "eMoonlitClouds"},
                                           {   eLingeringTunes,    "eLingeringTunes"},
                                           {      eEchoSetNone,       "eEchoSetNone"}
} )

constexpr std::array<std::tuple<int, int, int>, eEchoSetCount> EchoSetColorIndicators = {
    std::make_tuple( 66, 178, 255 ),
    std::make_tuple( 245, 118, 79 ),
    std::make_tuple( 182, 108, 255 ),
    std::make_tuple( 86, 255, 183 ),
    std::make_tuple( 247, 228, 107 ),
    std::make_tuple( 204, 141, 181 ),
    std::make_tuple( 135, 189, 41 ),
    std::make_tuple( 255, 255, 255 ),
    std::make_tuple( 202, 44, 37 ),
};


constexpr auto
ColorIndicatorsMinDifferent( ) noexcept
{
    int SmallestDistance = std::numeric_limits<int>::max( );
    for ( const auto& A : EchoSetColorIndicators )
    {
        for ( const auto& B : EchoSetColorIndicators )
        {
            const auto XDistance = std::get<0>( A ) - std::get<0>( B );
            const auto YDistance = std::get<1>( A ) - std::get<1>( B );
            const auto ZDistance = std::get<2>( A ) - std::get<2>( B );

            const auto Distance = XDistance * XDistance + YDistance * YDistance + ZDistance * ZDistance;
            if ( Distance != 0 && Distance < SmallestDistance )
            {
                SmallestDistance = Distance;
            }
        }
    }

    return SmallestDistance;
}

constexpr EchoSet
MatchColorToSet( std::tuple<int, int, int> Color ) noexcept
{
    int SmallestDistance   = std::numeric_limits<int>::max( );
    int SmallestDistanceAt = -1;
    for ( int i = 0; i < eEchoSetCount; ++i )
    {
        const auto XDistance = std::get<0>( EchoSetColorIndicators[ i ] ) - std::get<0>( Color );
        const auto YDistance = std::get<1>( EchoSetColorIndicators[ i ] ) - std::get<1>( Color );
        const auto ZDistance = std::get<2>( EchoSetColorIndicators[ i ] ) - std::get<2>( Color );

        const auto Distance = XDistance * XDistance + YDistance * YDistance + ZDistance * ZDistance;
        if ( Distance < SmallestDistance )
        {
            SmallestDistance   = Distance;
            SmallestDistanceAt = i;
        }
    }

    if ( SmallestDistance >= ColorIndicatorsMinDifferent( ) / 4 )
    {
        return eEchoSetNone;
    }

    return (EchoSet) SmallestDistanceAt;
}

struct MultiplierConfig {

    FloatTy auto_attack_multiplier  = 0;
    FloatTy heavy_attack_multiplier = 0;
    FloatTy skill_multiplier        = 0;
    FloatTy ult_multiplier          = 0;
};

struct EffectiveStats {

    EchoSet Set               = eFreezingFrost;
    int     NameID            = 0;
    int     Cost              = 0;
    FloatTy flat_attack       = 0;
    FloatTy regen             = 0;
    FloatTy percentage_attack = 0;
    FloatTy buff_multiplier   = 0;
    FloatTy crit_rate         = 0;
    FloatTy crit_damage       = 0;

    FloatTy auto_attack_buff  = 0;
    FloatTy heavy_attack_buff = 0;
    FloatTy skill_buff        = 0;
    FloatTy ult_buff          = 0;

    EffectiveStats& operator+=( const EffectiveStats& Other ) noexcept
    {
        Set    = eEchoSetNone;
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

    EffectiveStats operator+( const EffectiveStats& Other ) const noexcept
    {
        return EffectiveStats {
            eEchoSetNone,
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

    EffectiveStats operator-( const EffectiveStats& Other ) const noexcept
    {
        return EffectiveStats {
            eEchoSetNone,
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

    bool operator==( const EffectiveStats& Other ) const noexcept
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

    FloatTy CritRateStat( ) const noexcept
    {
        return crit_rate + 0.05f;
    }

    FloatTy CritDamageStat( ) const noexcept
    {
        return crit_damage + 1.5f;
    }

    FloatTy AttackStat( auto base_attack ) const noexcept
    {
        return base_attack * ( 1 + percentage_attack ) + flat_attack;
    }

    FloatTy NormalDamage( auto base_attack, const MultiplierConfig* multiplier_config ) const noexcept
    {
        // clang-format off
        return multiplier_config->auto_attack_multiplier  * AttackStat( base_attack ) * ( 1.f + buff_multiplier + auto_attack_buff  )
             + multiplier_config->heavy_attack_multiplier * AttackStat( base_attack ) * ( 1.f + buff_multiplier + heavy_attack_buff )
             + multiplier_config->skill_multiplier        * AttackStat( base_attack ) * ( 1.f + buff_multiplier + skill_buff        )
             + multiplier_config->ult_multiplier          * AttackStat( base_attack ) * ( 1.f + buff_multiplier + ult_buff          );
        // clang-format on
    }

    FloatTy CritDamage( auto base_attack, const MultiplierConfig* multiplier_config ) const noexcept
    {
        return NormalDamage( base_attack, multiplier_config ) * CritDamageStat( );
    }

    FloatTy ExpectedDamage( auto base_attack, const MultiplierConfig* multiplier_config ) const noexcept
    {
        return NormalDamage( base_attack, multiplier_config ) * ( 1 + std::min( CritRateStat( ), (FloatTy) 1 ) * ( 0.5f + crit_damage ) );
    }
};

enum StatType : char {
    eAttack,
    eAttackPercentage,
    eAutoAttackDamagePercentage,
    eHeavyAttackPercentage,
    eUltDamagePercentage,
    eSkillDamagePercentage,
    eHealBonusPercentage,
    eFireDamagePercentage,
    eAirDamagePercentage,
    eIceDamagePercentage,
    eElectricDamagePercentage,
    eDarkDamagePercentage,
    eLightDamagePercentage,
    eDefence,
    eDefencePercentage,
    eHealth,
    eHealthPercentage,
    eRegenPercentage,
    eCritDamage,
    eCritRate,
};

struct FullStats {
    int         Cost  = 1;
    int         Level = 0;
    EchoSet     Set   = eEchoSetNone;
    std::string EchoName;

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

        int StatIndex = 0;

        ss << std::format( "[{}]: {:26}: {:>21}\n", ++StatIndex, "EchoName", EchoName );
        if ( std::abs( AutoAttackDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "AutoAttackDamagePercentage", AutoAttackDamagePercentage * 100 );
        if ( std::abs( HeavyAttackPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "HeavyAttackPercentage", HeavyAttackPercentage * 100 );
        if ( std::abs( UltDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "UltDamagePercentage", UltDamagePercentage * 100 );
        if ( std::abs( SkillDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "SkillDamagePercentage", SkillDamagePercentage * 100 );
        if ( std::abs( HealBonusPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "HealBonusPercentage", HealBonusPercentage * 100 );
        if ( std::abs( FireDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "FireDamagePercentage", FireDamagePercentage * 100 );
        if ( std::abs( AirDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "AirDamagePercentage", AirDamagePercentage * 100 );
        if ( std::abs( IceDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "IceDamagePercentage", IceDamagePercentage * 100 );
        if ( std::abs( ElectricDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "ElectricDamagePercentage", ElectricDamagePercentage * 100 );
        if ( std::abs( DarkDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "DarkDamagePercentage", DarkDamagePercentage * 100 );
        if ( std::abs( LightDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "LightDamagePercentage", LightDamagePercentage * 100 );
        if ( std::abs( AttackPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "AttackPercentage", AttackPercentage * 100 );
        if ( std::abs( DefencePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "DefencePercentage", DefencePercentage * 100 );
        if ( std::abs( HealthPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "HealthPercentage", HealthPercentage * 100 );
        if ( std::abs( RegenPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "RegenPercentage", RegenPercentage * 100 );
        if ( std::abs( Attack ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "Flat Attack", Attack );
        if ( std::abs( Defence ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "Flat Defence", Defence );
        if ( std::abs( Health ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "Flat Health", Health );
        if ( std::abs( CritDamage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "CritDamage", CritDamage * 100 );
        if ( std::abs( CritRate ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, "CritRate", CritRate * 100 );

        return ss.str( );
    }
};

namespace NLOHMANN_utils
{
inline bool
ShouldSerialize( const auto& Data )
{
    if constexpr ( !std::is_floating_point_v<std::decay_t<decltype( Data )>> )
    {
        return true;
    } else
    {
        return std::abs( Data ) > 0.0001f;
    }
}
}   // namespace NLOHMANN_utils

#define NLOHMANN_JSON_TO_LAZY( v1 ) \
    if ( NLOHMANN_utils::ShouldSerialize( nlohmann_json_t.v1 ) ) nlohmann_json_j[ #v1 ] = nlohmann_json_t.v1;

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
                    EchoName,
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

#include "SetStat.hpp"

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
    } else if constexpr ( DamageType == eHeavyAttackPercentage )
    {
        return &FullStats::HeavyAttackPercentage;
    } else if constexpr ( DamageType == eSkillDamagePercentage )
    {
        return &FullStats::SkillDamagePercentage;
    } else if constexpr ( DamageType == eUltDamagePercentage )
    {
        return &FullStats::UltDamagePercentage;
    }

    std::unreachable( );
}

template <char ElementType>
EffectiveStats
ToEffectiveStats( const FullStats& MatchResult )
{
    return EffectiveStats {
        .Set               = MatchResult.Set,
        .NameID            = -1,
        .Cost              = MatchResult.Cost,
        .flat_attack       = MatchResult.Attack,
        .regen             = MatchResult.RegenPercentage,
        .percentage_attack = MatchResult.AttackPercentage,
        .buff_multiplier   = MatchResult.*GetElementBonusPtr<ElementType>( ),
        .crit_rate         = MatchResult.CritRate,
        .crit_damage       = MatchResult.CritDamage,
        .auto_attack_buff  = MatchResult.AutoAttackDamagePercentage,
        .heavy_attack_buff = MatchResult.HeavyAttackPercentage,
        .skill_buff        = MatchResult.SkillDamagePercentage,
        .ult_buff          = MatchResult.UltDamagePercentage };
}
