//
// Created by EMCJava on 7/26/2024.
//

#pragma once

#include <Common/Stat/FullStats.hpp>
#include <Common/Stat/EchoSet.hpp>

#include <Opt/SubStatRolling/SubStatRollConfig.hpp>
#include <Opt/Tweak/CombinationMetaCache.hpp>

#include <Loca/StringArrayObserver.hpp>
#include <Loca/Loca.hpp>

#include <vector>
#include <array>

struct EchoPotential {

    FloatTy BaselineOptimizingValue = { };
    FloatTy HighestOptimizingValue  = { };
    FloatTy LowestOptimizingValue   = { };

    static constexpr size_t            CDFResolution      = 500;
    std::vector<FloatTy>               CDFChangeToOV      = { };
    std::vector<FloatTy>               CDFSmallOrEqOV     = { };
    std::vector<FloatTy>               CDFFloat           = { };
    std::vector<ValueRollRate::RateTy> CDF                = { };
    ValueRollRate::RateTy              ExpectedChangeToOV = { };
};

class CombinationTweaker
{
protected:
    static constexpr int                             MaxRollCount                = 5;
    static constexpr int                             MaxNonEffectiveSubStatCount = 5;
    static std::array<std::array<long long, 14>, 14> PascalTriangle;

    const CombinationMetaCache m_TweakerTarget;

    int m_TweakingEchoSlot = 0;

    std::unique_ptr<EchoPotential> m_FullPotentialCache;

    std::unique_ptr<EchoPotential>
    CalculateFullPotential( int                                   EchoSlot,
                            EchoSet                               Set,
                            int                                   EchoNameID,
                            const StatValueConfig&                PrimaryStat,
                            const StatValueConfig&                SecondaryStat,
                            int                                   NonEffectiveRollCount = MaxNonEffectiveSubStatCount,
                            const std::vector<SubStatRollConfig>* RollConfigs           = &FullSubStatRollConfigs );

    void ClearPotentialCache( ) { m_FullPotentialCache.reset( ); }

    EchoPotential
    AnalyzeEchoPotential( std::vector<std::pair<FloatTy, ValueRollRate::RateTy>>& OptimizingValueDistribution );

    void
    ApplyStats(
        std::vector<std::pair<FloatTy, ValueRollRate::RateTy>>& Results,
        const SubStatRollConfig**                               PickedRollPtr,
        const EffectiveStats&                                   Stats,
        ValueRollRate::RateTy                                   CurrentRate = 1 );

    void
    CalculateEchoPotential(
        EchoPotential&         Result,
        int                    EchoSlot,
        EffectiveStats         CurrentSubStats,
        const StatValueConfig& PrimaryStat,
        const StatValueConfig& SecondaryStat,
        int                    RollRemaining );

public:
    CombinationTweaker( const CombinationMetaCache& Target );

    //  This will pick the highest main-stat to calculate the lowest
    std::pair<FloatTy, FloatTy> CalculateSubStatMinMaxExpectedDamage( int EchoSlot );
};

class CombinationTweakerMenu
    : public LanguageObserver
    , public CombinationTweaker
{
protected:
    int /* First time setting up main stat */ PreviousTweakingCost = -1;
    StringArrayObserver                       m_MainStatLabel;

    StringArrayObserver m_EchoNames;
    StringArrayObserver m_SetNames;
    StringArrayObserver m_SubStatLabel;

    std::array<FloatTy EffectiveStats::*, 9> m_SubStatPtr;

    /*
     *
     * Controls
     *
     * */
    int                            m_EchoSlotIndex      = -1;
    int                            m_UsedSubStatCount   = 0;
    std::array<StatValueConfig, 5> m_EchoSubStatConfigs = { };

    double             m_DragEDTargetY               = 0;
    int                m_SelectedMainStatTypeIndex   = -1;
    int                m_SelectedEchoNameID          = 0;
    int                m_SelectedEchoSet             = (int) EchoSet::eEchoSetNone;
    std::array<int, 5> m_SelectedSubStatTypeIndices  = { };
    std::array<int, 5> m_SelectedSubStatValueIndices = { };

    EchoPotential m_SelectedEchoPotential;

    std::optional<FloatTy> m_PinnedTargetCDF;
    std::optional<FloatTy> m_PinnedExpectedChange;

public:
    CombinationTweakerMenu( Loca& LanguageProvider, const CombinationMetaCache& Target );

    void TweakerMenu( const std::map<std::string, std::vector<std::string>>& EchoNamesBySet );
};