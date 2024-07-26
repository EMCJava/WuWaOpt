//
// Created by EMCJava on 7/26/2024.
//

#pragma once

#include <Opt/SubStatRolling/SubStatRollConfig.hpp>
#include <Opt/Tweak/CombinationMetaCache.hpp>
#include <Opt/FullStats.hpp>

#include <Loca/Loca.hpp>

#include <vector>
#include <array>

struct EchoPotential {

    FloatTy BaselineExpectedDamage = { };
    FloatTy HighestExpectedDamage  = { };
    FloatTy LowestExpectedDamage   = { };

    static constexpr size_t            CDFResolution  = 500;
    std::vector<FloatTy>               CDFChangeToED  = { };
    std::vector<FloatTy>               CDFSmallOrEqED = { };
    std::vector<FloatTy>               CDFFloat       = { };
    std::vector<ValueRollRate::RateTy> CDF            = { };
};

class CombinationTweaker
{
    static constexpr int MaxRollCount = 5;
    const Loca&          LanguageProvider;

    int                            m_EchoSlotIndex      = -1;
    int                            m_UsedSubStatCount   = 0;
    std::vector<const char*>       m_EchoNames          = { };
    std::array<StatValueConfig, 5> m_EchoSubStatConfigs = { };

    double             m_DragEDTargetY               = 0;
    int                m_SelectedMainStatTypeIndex   = -1;
    int                m_SelectedEchoNameID          = 0;
    int                m_SelectedEchoSet             = static_cast<EchoSet>( -1 );
    std::array<int, 5> m_SelectedSubStatTypeIndices  = { };
    std::array<int, 5> m_SelectedSubStatValueIndices = { };

    int                                       m_NonEffectiveSubStatCount;
    std::vector<SubStatRollConfig>            m_SubStatRollConfigs;
    std::array<std::array<long long, 14>, 14> m_PascalTriangle { };

    EchoPotential m_SelectedEchoPotential;

    std::vector<const char*>                 m_SetNames;
    std::array<FloatTy EffectiveStats::*, 9> m_SubStatPtr;
    std::array<const char*, 9>               m_SubStatLabel;

    void
    ApplyStats(
        std::vector<std::pair<FloatTy, ValueRollRate::RateTy>>& Results,
        CombinationMetaCache&                                   Environment,
        const SubStatRollConfig**                               PickedRollPtr,
        const EffectiveStats&                                   Stats,
        int                                                     SlotIndex,
        ValueRollRate::RateTy                                   CurrentRate = 1 );

    void
    CalculateEchoPotential(
        EchoPotential&                        Result,
        const std::vector<SubStatRollConfig>& RollConfigs,
        CombinationMetaCache&                 Environment,
        EffectiveStats                        CurrentSubStats,
        const StatValueConfig&                FirstMainStat,
        const StatValueConfig&                SecondMainStat,
        int                                   RollRemaining,
        int                                   SlotIndex );

    void InitializeSubStatRollConfigs( );
    void InitializePascalTriangle( );

public:
    CombinationTweaker( const Loca& LanguageProvider );

    void TweakerMenu( CombinationMetaCache&                                  Target,
                      const std::map<std::string, std::vector<std::string>>& EchoNamesBySet );
};
