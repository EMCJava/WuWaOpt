//
// Created by EMCJava on 6/2/2024.
//

#pragma once

using FloatTy = float;

enum EchoSet : uint8_t {
    eFreezingFrost,
    eMoltenRift,
    eVoidThunder,
    eSierraGale,
    eCelestialLight,
    eSunSinkingEclipses,
    eRejuvenatingGlow,
    eMoonlitClouds,
    eLingeringTunes,
    eEchoSetCount,
    eEchoSetNone = eEchoSetCount
};

NLOHMANN_JSON_SERIALIZE_ENUM( EchoSet, {
                                           {     eFreezingFrost,      "eFreezingFrost"},
                                           {        eMoltenRift,         "eMoltenRift"},
                                           {       eVoidThunder,        "eVoidThunder"},
                                           {        eSierraGale,         "eSierraGale"},
                                           {    eCelestialLight,     "eCelestialLight"},
                                           {eSunSinkingEclipses, "eSunSinkingEclipses"},
                                           {  eRejuvenatingGlow,   "eRejuvenatingGlow"},
                                           {     eMoonlitClouds,      "eMoonlitClouds"},
                                           {    eLingeringTunes,     "eLingeringTunes"},
                                           {       eEchoSetNone,        "eEchoSetNone"}
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

struct EffectiveStats {

    EchoSet Set               = eFreezingFrost;
    int     Cost              = 0;
    FloatTy flat_attack       = 0;
    FloatTy percentage_attack = 0;
    FloatTy buff_multiplier   = 0;
    FloatTy crit_rate         = 0;
    FloatTy crit_damage       = 0;

    EffectiveStats& operator+=( const EffectiveStats& Other ) noexcept
    {
        Set = eEchoSetNone;

        Cost += Other.Cost;
        flat_attack += Other.flat_attack;
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
        .percentage_attack = MatchResult.AttackPercentage,
        .buff_multiplier =
            MatchResult.*GetElementBonusPtr<ElementType>( )
            + MatchResult.*GetSkillBonusPtr<DamageType>( ),
        .crit_rate   = MatchResult.CritRate,
        .crit_damage = MatchResult.CritDamage };
}
