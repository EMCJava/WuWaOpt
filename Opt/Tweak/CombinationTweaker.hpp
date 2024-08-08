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

    FloatTy BaselineExpectedDamage = { };
    FloatTy HighestExpectedDamage  = { };
    FloatTy LowestExpectedDamage   = { };

    static constexpr size_t            CDFResolution      = 500;
    std::vector<FloatTy>               CDFChangeToED      = { };
    std::vector<FloatTy>               CDFSmallOrEqED     = { };
    std::vector<FloatTy>               CDFFloat           = { };
    std::vector<ValueRollRate::RateTy> CDF                = { };
    ValueRollRate::RateTy              ExpectedChangeToED = { };
};

class CombinationTweaker : public LanguageObserver
{
    static constexpr int MaxRollCount = 5;

    const CombinationMetaCache m_TweakerTarget;

    int                            m_EchoSlotIndex      = -1;
    int                            m_UsedSubStatCount   = 0;
    std::array<StatValueConfig, 5> m_EchoSubStatConfigs = { };

    double             m_DragEDTargetY               = 0;
    int                m_SelectedMainStatTypeIndex   = -1;
    int                m_SelectedEchoNameID          = 0;
    int                m_SelectedEchoSet             = (int) EchoSet::eEchoSetNone;
    std::array<int, 5> m_SelectedSubStatTypeIndices  = { };
    std::array<int, 5> m_SelectedSubStatValueIndices = { };

    std::optional<FloatTy> m_PinnedTargetCDF;
    std::optional<FloatTy> m_PinnedExpectedChange;

    int m_NonEffectiveSubStatCount = 5;

    std::array<std::array<long long, 14>, 14> m_PascalTriangle { };

    EchoPotential m_SelectedEchoPotential;

    StringArrayObserver                      m_EchoNames;
    StringArrayObserver                      m_SetNames;
    StringArrayObserver                      m_SubStatLabel;
    std::array<FloatTy EffectiveStats::*, 9> m_SubStatPtr;

    int /* First time setting up main stat */ PreviousTweakingCost = -1;
    StringArrayObserver                       m_MainStatLabel;

    std::unique_ptr<EchoPotential> m_FullPotentialCache;

    void CalculateFullPotential( );
    void ClearPotentialCache( )
    {
        m_FullPotentialCache.reset( );
    }

    EchoPotential
    AnalyzeEchoPotential( std::vector<std::pair<FloatTy, ValueRollRate::RateTy>>& DamageDistribution );

    void
    ApplyStats(
        std::vector<std::pair<FloatTy, ValueRollRate::RateTy>>& Results,
        const SubStatRollConfig**                               PickedRollPtr,
        const EffectiveStats&                                   Stats,
        ValueRollRate::RateTy                                   CurrentRate = 1 );


    void
    CalculateEchoPotential(
        EchoPotential& Result,
        EffectiveStats CurrentSubStats,
        int            RollRemaining );

    void InitializePascalTriangle( );

public:
    CombinationTweaker( Loca& LanguageProvider, CombinationMetaCache Target );

    void TweakerMenu( const std::map<std::string, std::vector<std::string>>& EchoNamesBySet );
};
