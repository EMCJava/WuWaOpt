//
// Created by EMCJava on 6/16/2024.
//

#pragma once

#include "OptUtil.hpp"

#include <vector>
#include <queue>
#include <array>
#include <set>

#define CostSlotTemplate template <int CostAt1, int CostAt2, int CostAt3, int CostAt4, int CostAt5>
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
    CostSlotTemplate void
    Run( int GAReportIndex, FloatTy BaseAttack );

public:
    explicit WuWaGA( auto& Echos )
        : m_Echos( Echos )
    { }

    [[nodiscard]] auto& GetReport( ) const noexcept
    {
        return m_GAReport;
    }

    void Run( );

protected:
    GARuntimeReport m_GAReport;

    int m_ResultLength   = 10;
    int m_PopulationSize = 10000, m_ReproduceSize = 1000;

    std::vector<std::unique_ptr<std::jthread>> m_Threads;


    const std::vector<EffectiveStats>& m_Echos;
};
