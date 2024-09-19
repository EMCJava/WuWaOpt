//
// Created by EMCJava on 8/5/2024.
//

#include "FullStats.hpp"

#include <magic_enum.hpp>

#include <sstream>

std::string_view
FullStats::GetSetName( ) const noexcept
{
    return magic_enum::enum_name( Set );
}

std::string
FullStats::BriefStat( const Loca& L ) const noexcept
{
    return std::format( "Cost: {} {:5}: {:2}",
                        Cost,
                        L[ "Level" ],
                        Level );
}

std::string
FullStats::DetailStat( const Loca& L ) const noexcept
{
    std::stringstream ss;

    int StatIndex = 0;

    ss << std::format( "[{}]: {:26}: {:>21}\n", ++StatIndex, L[ "EchoName" ], L[ EchoName ] );
    if ( std::abs( AutoAttackDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "AutoAttack%" ], AutoAttackDamagePercentage * 100 );
    if ( std::abs( HeavyAttackPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "HeavyAttack%" ], HeavyAttackPercentage * 100 );
    if ( std::abs( UltDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "UltDamage%" ], UltDamagePercentage * 100 );
    if ( std::abs( SkillDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "SkillDamage%" ], SkillDamagePercentage * 100 );
    if ( std::abs( HealBonusPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "Heal%" ], HealBonusPercentage * 100 );
    if ( std::abs( FireDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "Fire%" ], FireDamagePercentage * 100 );
    if ( std::abs( AirDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "Air%" ], AirDamagePercentage * 100 );
    if ( std::abs( IceDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "Ice%" ], IceDamagePercentage * 100 );
    if ( std::abs( ElectricDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "Elec%" ], ElectricDamagePercentage * 100 );
    if ( std::abs( DarkDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "Dark%" ], DarkDamagePercentage * 100 );
    if ( std::abs( LightDamagePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "Light%" ], LightDamagePercentage * 100 );
    if ( std::abs( AttackPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "Attack%" ], AttackPercentage * 100 );
    if ( std::abs( DefencePercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "Defence%" ], DefencePercentage * 100 );
    if ( std::abs( HealthPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "Health%" ], HealthPercentage * 100 );
    if ( std::abs( RegenPercentage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "Regen%" ], RegenPercentage * 100 );
    if ( std::abs( Attack ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "FlatAttack" ], Attack );
    if ( std::abs( Defence ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "FlatDefence" ], Defence );
    if ( std::abs( Health ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "FlatHealth" ], Health );
    if ( std::abs( CritDamage ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "CritDamage" ], CritDamage * 100 );
    if ( std::abs( CritRate ) > 0.001 ) ss << std::format( "[{}]: {:26}: {:>21.1f}\n", ++StatIndex, L[ "CritRate" ], CritRate * 100 );

    return ss.str( );
}

namespace std
{
namespace
{

    // Code from boost
    // Reciprocal of the golden ratio helps spread entropy
    //     and handles duplicates.
    // See Mike Seymour in magic-numbers-in-boosthash-combine:
    //     https://stackoverflow.com/questions/4948780

    template <class T>
    inline void hash_combine( std::size_t& seed, T const& v )
    {
        seed ^= hash<T>( )( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    }

    // Recursive template code derived from Matthieu M.
    template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
    struct HashValueImpl {
        static void apply( size_t& seed, Tuple const& tuple )
        {
            HashValueImpl<Tuple, Index - 1>::apply( seed, tuple );
            hash_combine( seed, get<Index>( tuple ) );
        }
    };

    template <class Tuple>
    struct HashValueImpl<Tuple, 0> {
        static void apply( size_t& seed, Tuple const& tuple )
        {
            hash_combine( seed, get<0>( tuple ) );
        }
    };
}   // namespace

template <typename... TT>
struct hash<std::tuple<TT...>> {
    size_t
    operator( )( std::tuple<TT...> const& tt ) const
    {
        size_t seed = 0;
        HashValueImpl<std::tuple<TT...>>::apply( seed, tt );
        return seed;
    }
};
}   // namespace std

std::size_t
FullStats::Hash( ) const noexcept
{
    const auto reflect = std::make_tuple( Cost,
                                          Set,
                                          EchoName,
                                          int( 1000 * AutoAttackDamagePercentage ),
                                          int( 1000 * HeavyAttackPercentage ),
                                          int( 1000 * UltDamagePercentage ),
                                          int( 1000 * SkillDamagePercentage ),
                                          int( 1000 * HealBonusPercentage ),
                                          int( 1000 * FireDamagePercentage ),
                                          int( 1000 * AirDamagePercentage ),
                                          int( 1000 * IceDamagePercentage ),
                                          int( 1000 * ElectricDamagePercentage ),
                                          int( 1000 * DarkDamagePercentage ),
                                          int( 1000 * LightDamagePercentage ),
                                          int( 1000 * AttackPercentage ),
                                          int( 1000 * DefencePercentage ),
                                          int( 1000 * HealthPercentage ),
                                          int( 1000 * RegenPercentage ),
                                          int( Attack ),
                                          int( Defence ),
                                          int( Health ),
                                          int( 1000 * CritDamage ),
                                          int( 1000 * CritRate ) );

    return std::hash<std::remove_cvref_t<decltype( reflect )>>( )( reflect );
}

YAML::Node
ToNode( const FullStats& rhs ) noexcept
{
    YAML::Node Node;

    Node[ "Cost" ]     = rhs.Cost;
    Node[ "Level" ]    = rhs.Level;
    Node[ "EchoName" ] = rhs.EchoName;
    Node[ "Set" ]      = std::string( rhs.GetSetName( ) );

    if ( std::abs( rhs.AutoAttackDamagePercentage ) > 0.001 ) Node[ "AutoAttackDamagePercentage" ] = std::format( "{}", rhs.AutoAttackDamagePercentage );
    if ( std::abs( rhs.HeavyAttackPercentage ) > 0.001 ) Node[ "HeavyAttackPercentage" ] = std::format( "{}", rhs.HeavyAttackPercentage );
    if ( std::abs( rhs.UltDamagePercentage ) > 0.001 ) Node[ "UltDamagePercentage" ] = std::format( "{}", rhs.UltDamagePercentage );
    if ( std::abs( rhs.SkillDamagePercentage ) > 0.001 ) Node[ "SkillDamagePercentage" ] = std::format( "{}", rhs.SkillDamagePercentage );
    if ( std::abs( rhs.HealBonusPercentage ) > 0.001 ) Node[ "HealBonusPercentage" ] = std::format( "{}", rhs.HealBonusPercentage );
    if ( std::abs( rhs.FireDamagePercentage ) > 0.001 ) Node[ "FireDamagePercentage" ] = std::format( "{}", rhs.FireDamagePercentage );
    if ( std::abs( rhs.AirDamagePercentage ) > 0.001 ) Node[ "AirDamagePercentage" ] = std::format( "{}", rhs.AirDamagePercentage );
    if ( std::abs( rhs.IceDamagePercentage ) > 0.001 ) Node[ "IceDamagePercentage" ] = std::format( "{}", rhs.IceDamagePercentage );
    if ( std::abs( rhs.ElectricDamagePercentage ) > 0.001 ) Node[ "ElectricDamagePercentage" ] = std::format( "{}", rhs.ElectricDamagePercentage );
    if ( std::abs( rhs.DarkDamagePercentage ) > 0.001 ) Node[ "DarkDamagePercentage" ] = std::format( "{}", rhs.DarkDamagePercentage );
    if ( std::abs( rhs.LightDamagePercentage ) > 0.001 ) Node[ "LightDamagePercentage" ] = std::format( "{}", rhs.LightDamagePercentage );
    if ( std::abs( rhs.AttackPercentage ) > 0.001 ) Node[ "AttackPercentage" ] = std::format( "{}", rhs.AttackPercentage );
    if ( std::abs( rhs.DefencePercentage ) > 0.001 ) Node[ "DefencePercentage" ] = std::format( "{}", rhs.DefencePercentage );
    if ( std::abs( rhs.HealthPercentage ) > 0.001 ) Node[ "HealthPercentage" ] = std::format( "{}", rhs.HealthPercentage );
    if ( std::abs( rhs.RegenPercentage ) > 0.001 ) Node[ "RegenPercentage" ] = std::format( "{}", rhs.RegenPercentage );
    if ( std::abs( rhs.Attack ) > 0.001 ) Node[ "Attack" ] = std::format( "{}", rhs.Attack );
    if ( std::abs( rhs.Defence ) > 0.001 ) Node[ "Defence" ] = std::format( "{}", rhs.Defence );
    if ( std::abs( rhs.Health ) > 0.001 ) Node[ "Health" ] = std::format( "{}", rhs.Health );
    if ( std::abs( rhs.CritDamage ) > 0.001 ) Node[ "CritDamage" ] = std::format( "{}", rhs.CritDamage );
    if ( std::abs( rhs.CritRate ) > 0.001 ) Node[ "CritRate" ] = std::format( "{}", rhs.CritRate );

    if ( !rhs.Occupation.empty( ) ) Node[ "Occupation" ] = rhs.Occupation;
    Node[ "EchoHash" ] = rhs.EchoHash == 0 ? rhs.Hash( ) : rhs.EchoHash;

    return Node;
}

bool
FromNode( const YAML::Node& Node, FullStats& rhs ) noexcept
{
    rhs.Cost     = Node[ "Cost" ].as<int>( );
    rhs.Level    = Node[ "Level" ].as<int>( );
    rhs.EchoName = Node[ "EchoName" ].as<std::string>( );
    rhs.Set      = magic_enum::enum_cast<EchoSet>( Node[ "Set" ].as<std::string>( ) ).value_or( EchoSet::eEchoSetNone );

    if ( const auto Value = Node[ "AutoAttackDamagePercentage" ]; Value ) rhs.AutoAttackDamagePercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "HeavyAttackPercentage" ]; Value ) rhs.HeavyAttackPercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "UltDamagePercentage" ]; Value ) rhs.UltDamagePercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "SkillDamagePercentage" ]; Value ) rhs.SkillDamagePercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "HealBonusPercentage" ]; Value ) rhs.HealBonusPercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "FireDamagePercentage" ]; Value ) rhs.FireDamagePercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "AirDamagePercentage" ]; Value ) rhs.AirDamagePercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "IceDamagePercentage" ]; Value ) rhs.IceDamagePercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "ElectricDamagePercentage" ]; Value ) rhs.ElectricDamagePercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "DarkDamagePercentage" ]; Value ) rhs.DarkDamagePercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "LightDamagePercentage" ]; Value ) rhs.LightDamagePercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "AttackPercentage" ]; Value ) rhs.AttackPercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "DefencePercentage" ]; Value ) rhs.DefencePercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "HealthPercentage" ]; Value ) rhs.HealthPercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "RegenPercentage" ]; Value ) rhs.RegenPercentage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "Attack" ]; Value ) rhs.Attack = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "Defence" ]; Value ) rhs.Defence = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "Health" ]; Value ) rhs.Health = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "CritDamage" ]; Value ) rhs.CritDamage = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "CritRate" ]; Value ) rhs.CritRate = Value.as<FloatTy>( );
    if ( const auto Value = Node[ "Occupation" ]; Value ) rhs.Occupation = Value.as<std::string>( );
    if ( const auto Value = Node[ "EchoHash" ]; Value ) rhs.EchoHash = Value.as<std::size_t>( );
    rhs.EchoHash = rhs.EchoHash == 0 ? rhs.Hash( ) : rhs.EchoHash;

    return true;
}
