//
// Created by EMCJava on 6/16/2024.
//

#pragma once

#include "OptUtil.hpp"

#include <vector>
#include <queue>
#include <array>
#include <set>

#define CostSlotTemplate         int CostAt1, int CostAt2, int CostAt3, int CostAt4, int CostAt5
#define CostSlotTemplateArgument CostAt1, CostAt2, CostAt3, CostAt4, CostAt5

struct GARuntimeReport {
    static constexpr auto MaxCombinationCount = 11;

    FloatTy OptimalValue[ MaxCombinationCount ] { };
    FloatTy MutationProb[ MaxCombinationCount ] { };

    std::array<std::vector<int>, MaxCombinationCount> ParentPickCount;

    struct DetailReportQueue {
        std::set<uint64_t> QueueSet;
        std::priority_queue<CombinationRecord,
                            std::vector<CombinationRecord>,
                            CombinationRecord>
            Queue;
    };

    std::array<DetailReportQueue, MaxCombinationCount> DetailReports;
};

// Genetic Algorithm for Wuthering Waves' echo system
class WuWaGA
{
private:
    template <char ElementType, char DamageType, CostSlotTemplate>
    void
    Run( int GAReportIndex, FloatTy BaseAttack );

    template <char ElementType, char DamageType, int SlotCount>
    inline FloatTy
    CalculateFitness( const std::array<int, SlotCount>& EffectiveCombination, const FloatTy BaseAttack );

public:
    static constexpr int ResultLength = 10;

    explicit WuWaGA( auto& Echos )
        : m_Echos( Echos )
    { }

    [[nodiscard]] auto& GetReport( ) const noexcept
    {
        return m_GAReport;
    }

    template <char ElementType, char DamageType>
    void Run( FloatTy BaseAttack );

protected:
    GARuntimeReport m_GAReport;

    int m_PopulationSize = 10000, m_ReproduceSize = 1000;

    std::vector<std::unique_ptr<std::jthread>> m_Threads;

    const std::vector<FullStats>& m_Echos;
    std::vector<EffectiveStats>   m_EffectiveEchos;
};

#include "WuWaGa.inl"