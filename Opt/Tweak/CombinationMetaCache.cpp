//
// Created by EMCJava on 7/26/2024.
//

#include "CombinationMetaCache.hpp"

#include <Opt/OptimizerParmSwitcher.hpp>

CombinationMetaCache::CombinationMetaCache( const std::vector<FullStats>& FullEchoList, const std::vector<EffectiveStats>& EffectiveEchoList )
    : FullEchoList( FullEchoList )
    , EffectiveEchoList( EffectiveEchoList )
{
}

void
CombinationMetaCache::Deactivate( )
{
    m_Valid            = false;
    m_CombinationStats = m_DisplayStats = m_IncreasePayOff = { };
}

void
CombinationMetaCache::SetAsCombination( const EffectiveStats& CS, int EO, int CI, int R, const PlotCombinationMeta& CombinationDetails, const OptimizerConfig& Config )
{
    m_CostCombinationTypeIndex = CI;
    m_Rank                     = R;
    SlotCount                = WuWaGA::GetCombinationLength( m_CostCombinationTypeIndex );

    std::array<int, 5> NewEchoIndices { };
    for ( int i = 0; i < SlotCount; ++i )
        NewEchoIndices[ i ] = CombinationDetails.Indices[ i ];

    // Check if the combination has changed
    if ( m_Valid && m_ElementOffset == EO && m_CachedConfigStateID == Config.InternalStateID && std::ranges::equal( NewEchoIndices, m_CombinationEchoIndices ) ) return;
    m_CombinationEchoIndices = NewEchoIndices;
    m_CommonStats            = CS;
    m_CachedConfigStateID    = Config.InternalStateID;
    m_OptimizerCfg           = Config;

    m_ElementOffset = EO;
    m_Valid         = true;

    const auto IndexToEchoTransform =
        std::views::transform( [ & ]( int EchoIndex ) -> EffectiveStats {
            return EffectiveEchoList[ m_CombinationEchoIndices[ EchoIndex ] ];
        } );

    m_Echoes =
        std::views::iota( 0, SlotCount )
        | IndexToEchoTransform
        | std::ranges::to<std::vector>( );

    m_EchoNames =
        std::views::iota( 0, SlotCount )
        | std::views::transform( [ & ]( int EchoIndex ) {
              return FullEchoList[ m_CombinationEchoIndices[ EchoIndex ] ].EchoName;
          } )
        | std::ranges::to<std::vector>( );

    m_CombinationStats =
        OptimizerParmSwitcher::SwitchCalculateCombinationalStat(
            m_ElementOffset,
            m_Echoes,
            m_CommonStats );

    for ( int i = 0; i < SlotCount; ++i )
    {
        m_EchoesWithoutAt[ i ] =
            std::views::iota( 0, SlotCount )
            | std::views::filter( [ i ]( int EchoIndex ) { return EchoIndex != i; } )
            | IndexToEchoTransform
            | std::ranges::to<std::vector>( );
    }

    m_DisplayStats.flat_attack       = m_CombinationStats.flat_attack;
    m_DisplayStats.regen             = m_CombinationStats.regen * 100 + 100;
    m_DisplayStats.percentage_attack = m_CombinationStats.percentage_attack * 100;
    m_DisplayStats.buff_multiplier   = m_CombinationStats.buff_multiplier * 100;
    m_DisplayStats.auto_attack_buff  = m_CombinationStats.auto_attack_buff * 100;
    m_DisplayStats.heavy_attack_buff = m_CombinationStats.heavy_attack_buff * 100;
    m_DisplayStats.skill_buff        = m_CombinationStats.skill_buff * 100;
    m_DisplayStats.ult_buff          = m_CombinationStats.ult_buff * 100;
    m_DisplayStats.crit_rate         = m_CombinationStats.CritRateStat( ) * 100;
    m_DisplayStats.crit_damage       = m_CombinationStats.CritDamageStat( ) * 100;
    m_FinalAttack                    = m_CombinationStats.AttackStat( Config.GetBaseAttack( ) );

    CalculateDamages( );
}

void
CombinationMetaCache::CalculateDamages( )
{
    const FloatTy Resistances = m_OptimizerCfg.GetResistances( );
    const FloatTy BaseAttack  = m_OptimizerCfg.GetBaseAttack( );

    m_CombinationStats.ExpectedDamage( BaseAttack, &m_OptimizerCfg.OptimizeMultiplierConfig,
                                       m_NormalDamage,
                                       m_CritDamage,
                                       m_ExpectedDamage );

    for ( int i = 0; i < SlotCount; ++i )
    {
        m_EDWithoutAt[ i ] =
            OptimizerParmSwitcher::SwitchCalculateCombinationalStat(
                m_ElementOffset,
                m_EchoesWithoutAt[ i ],
                m_CommonStats )
                .ExpectedDamage( BaseAttack, &m_OptimizerCfg.OptimizeMultiplierConfig );

        m_EdDropPercentageWithoutAt[ i ] = m_ExpectedDamage - m_EDWithoutAt[ i ];
    }

    const auto ExpectedDamageDrops = m_EdDropPercentageWithoutAt | std::views::take( SlotCount );
    const auto MaxDropOff          = std::ranges::max( ExpectedDamageDrops );
    const auto MinDropOff          = std::ranges::min( ExpectedDamageDrops );
    const auto DropOffRange        = MaxDropOff - MinDropOff;

    std::ranges::copy( ExpectedDamageDrops
                           | std::views::transform( [ & ]( FloatTy DropOff ) {
                                 return 1 - ( DropOff - MinDropOff ) / DropOffRange;
                             } ),
                       m_EdDropPercentageWithoutAt.begin( ) );

    static constexpr std::array<FloatTy EffectiveStats::*, 9> PercentageStats {
        &EffectiveStats::regen,
        &EffectiveStats::percentage_attack,
        &EffectiveStats::buff_multiplier,
        &EffectiveStats::crit_rate,
        &EffectiveStats::crit_damage,
        &EffectiveStats::auto_attack_buff,
        &EffectiveStats::heavy_attack_buff,
        &EffectiveStats::skill_buff,
        &EffectiveStats::ult_buff,
    };

    FloatTy MaxDamageBuff = 0;

    {
        auto NewStat = m_CombinationStats;
        NewStat.flat_attack += 1;

        const auto NewExpDmg = NewStat.ExpectedDamage( BaseAttack, &m_OptimizerCfg.OptimizeMultiplierConfig );
        MaxDamageBuff        = std::max( m_IncreasePayOff.flat_attack = NewExpDmg - m_ExpectedDamage, MaxDamageBuff );
    }

    for ( auto StatSlot : PercentageStats )
    {
        auto NewStat = m_CombinationStats;
        NewStat.*StatSlot += 0.01f;

        const auto NewExpDmg = NewStat.ExpectedDamage( BaseAttack, &m_OptimizerCfg.OptimizeMultiplierConfig );
        MaxDamageBuff        = std::max( m_IncreasePayOff.*StatSlot = NewExpDmg - m_ExpectedDamage, MaxDamageBuff );
    }

    m_IncreasePayOffWeight.flat_attack = m_IncreasePayOff.flat_attack / MaxDamageBuff;
    for ( auto StatSlot : PercentageStats )
        m_IncreasePayOffWeight.*StatSlot = m_IncreasePayOff.*StatSlot / MaxDamageBuff;

    m_NormalDamage *= Resistances;
    m_CritDamage *= Resistances;
    m_ExpectedDamage *= Resistances;
}

FloatTy
CombinationMetaCache::GetEDReplaceEchoAt( int EchoIndex, EffectiveStats Echo )
{
    const FloatTy Resistances = m_OptimizerCfg.GetResistances( );
    const FloatTy BaseAttack  = m_OptimizerCfg.GetBaseAttack( );

    // I don't like this
    m_EchoesWithoutAt[ EchoIndex ].push_back( Echo );
    const auto NewED =
        OptimizerParmSwitcher::SwitchCalculateCombinationalStat(
            m_ElementOffset,
            m_EchoesWithoutAt[ EchoIndex ],
            m_CommonStats )
            .ExpectedDamage( BaseAttack, &m_OptimizerCfg.OptimizeMultiplierConfig )
        * Resistances;
    m_EchoesWithoutAt[ EchoIndex ].pop_back( );

    return NewED;
}

bool
CombinationMetaCache::operator==( const CombinationMetaCache& Other ) const noexcept
{
    return m_Valid == Other.m_Valid && m_CostCombinationTypeIndex == Other.m_CostCombinationTypeIndex && m_Rank == Other.m_Rank && m_ElementOffset == Other.m_ElementOffset;
}