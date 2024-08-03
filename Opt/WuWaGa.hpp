//
// Created by EMCJava on 6/16/2024.
//

#pragma once

#include "Config/EchoConstraint.hpp"
#include "OptUtil.hpp"

#include <stop_token>
#include <vector>
#include <queue>
#include <array>
#include <set>

#define CostSlotTemplate         int CostAt1, int CostAt2, int CostAt3, int CostAt4, int CostAt5
#define CostSlotTemplateArgument CostAt1, CostAt2, CostAt3, CostAt4, CostAt5

struct GARuntimeReport {
    static constexpr auto MaxCombinationCount = 11;

    std::array<FloatTy, MaxCombinationCount> OptimalValue { 0 };
    std::array<FloatTy, MaxCombinationCount> MutationProb { 0 };

    struct DetailReportQueue {
        std::mutex         ReportLock;
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
    template <char ElementType, CostSlotTemplate>
    inline void
    Run( std::stop_token StopToken, int GAReportIndex, FloatTy BaseAttack, EffectiveStats CommonStats, const MultiplierConfig* OptimizeMultiplierConfig, const EchoConstraint& Constraints );

public:
    constexpr static std::array<const char*, 11> CombinationLabels { "444", "4431", "3333", "44111", "41111", "43311", "43111", "31111", "33111", "33311", "11111" };

    static constexpr int GetCombinationLength( int Index ) { return std::char_traits<char>::length( CombinationLabels[ Index ] ); }

    static constexpr int ResultLength = 30;

    [[nodiscard]] auto& GetReport( ) noexcept { return m_GAReport; }

    template <char ElementType>
    inline void Run( FloatTy BaseAttack, const EffectiveStats& CommonStats, const MultiplierConfig* OptimizeMultiplierConfig, const EchoConstraint& Constraints );

    void SetEchoes( const auto& Echoes ) noexcept { m_Echos = &Echoes; }

    [[nodiscard]] auto& GetEffectiveEchos( ) const noexcept { return m_EffectiveEchos; }

    [[nodiscard]] inline bool IsRunning( ) const;

    void Stop( ) { m_Threads.clear( ); }

protected:
    GARuntimeReport m_GAReport;

#ifdef _DEBUG
    int m_PopulationSize = 2000;
#else
    int m_PopulationSize = 200000;
#endif

    int m_ReproduceSize = 0.2 * m_PopulationSize;

    std::vector<FullStats> const* m_Echos = nullptr;
    std::vector<EffectiveStats>   m_EffectiveEchos;

    std::vector<std::unique_ptr<std::jthread>> m_Threads;
};

#include "WuWaGa.inl"