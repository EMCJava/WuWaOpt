//
// Created by EMCJava on 6/16/2024.
//

#include "Random.hpp"

#include <source_location>
#include <unordered_map>
#include <unordered_set>
#include <stop_token>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <numeric>
#include <vector>
#include <ranges>
#include <random>

enum ECostToIndex {
    eCost1 = 0,
    eCost3 = 1,
    eCost4 = 2,
    eMaxCostIndex
};

inline constexpr ECostToIndex
CostToIndex( int Cost )
{
    __assume( Cost == 1 || Cost == 3 || Cost == 4 );

    if ( Cost == 1 ) return eCost1;
    if ( Cost == 3 ) return eCost3;
    if ( Cost == 4 ) return eCost4;

    std::unreachable( );
}

template <CostSlotTemplate>
inline constexpr int
GetSlotCount( )
{
    constexpr std::array<int, 5> Costs { CostSlotTemplateArgument };
    for ( int i = 0; i < 5; ++i )
        if ( Costs[ i ] == 0 ) return i;
    return 5;
}

template <CostSlotTemplate>
inline constexpr int
GetCountByFixedCost( int Cost )
{
    constexpr std::array<int, 5> Costs { CostSlotTemplateArgument };

    int Result = 0;
    for ( int i = 0; i < 5; ++i )
        if ( Costs[ i ] == Cost ) ++Result;
    return Result;
}

template <CostSlotTemplate>
inline constexpr std::pair<int, int>
GetLowerBoundAndCountByFixedCost( int Cost )
{
    constexpr std::array<int, 5> Costs { CostSlotTemplateArgument };

    int LowerBound = -1;
    int Count      = 0;
    for ( int i = 0; i < 5; ++i )
    {
        if ( Costs[ i ] == Cost )
        {
            if ( LowerBound == -1 )
            {
                LowerBound = i;
                ++Count;
            } else
                ++Count;
        }
    }
    return { LowerBound == -1 ? 0 : LowerBound, Count };
}

template <CostSlotTemplate>
inline constexpr int
GetCostAt( int Index )
{
    constexpr std::array<int, 5> Costs { CostSlotTemplateArgument };
    return Costs[ Index ];
}

inline constexpr auto
Factorial( auto X )
{
    if ( 0 == X )
        return 1;
    return X * Factorial( X - 1 );
}

template <int SlotCount>
inline constexpr auto
GeneratePermutations( )
{
    constexpr auto MaxPermutation = Factorial( SlotCount );

    std::vector<std::array<int, SlotCount>> IndicesPermutations;
    IndicesPermutations.reserve( MaxPermutation * SlotCount );
    {
        std::array<int, SlotCount> Permutation;
        std::ranges::iota( Permutation, 0 );
        do
        {
            IndicesPermutations.push_back( Permutation );
        } while ( std::ranges::next_permutation( Permutation ).found );
    }

    return IndicesPermutations;
}

template <typename Ty, int BufferSize>
struct PreAllocatedBuffer {
    std::array<Ty, BufferSize> Buffer { };
    int                        Size = 0;

    void Clear( ) noexcept
    {
        Size = 0;
    }

    void PushBack( auto&& Value )
    {
        Buffer[ Size++ ] = Value;
    }

    decltype( auto ) operator[]( int Index ) noexcept
    {
        return Buffer[ Index ];
    }

    decltype( auto ) operator[]( int Index ) const noexcept
    {
        return Buffer[ Index ];
    }
};

template <ElementType ETy, CostSlotTemplate>
inline void
WuWaGA::Run( std::stop_token StopToken, int GAReportIndex, FloatTy BaseAttack, EffectiveStats CommonStats, const SkillMultiplierConfig* OptimizeMultiplierConfig, const EchoConstraint& Constraints )
{
    const auto SL = std::source_location::current( );

    static constexpr int MaxEchoCount     = 2000;
    static constexpr int IndexBitsShift   = 11;
    const auto           ReproduceSizeBy5 = m_ReproduceSize / 5;

    constexpr int SlotCount = GetSlotCount<CostSlotTemplateArgument>( );
    static_assert( ( 1 << IndexBitsShift ) > MaxEchoCount && IndexBitsShift * SlotCount <= 64 );
    constexpr auto MaxPermutation = Factorial( SlotCount );

    constexpr std::array<int, eMaxCostIndex>
        CountByFixedCost {
            GetCountByFixedCost<CostSlotTemplateArgument>( 1 ),
            GetCountByFixedCost<CostSlotTemplateArgument>( 3 ),
            GetCountByFixedCost<CostSlotTemplateArgument>( 4 ) };

    constexpr std::array<std::pair<int, int>, eMaxCostIndex>
        LowerBoundAndCountByFixedCost {
            GetLowerBoundAndCountByFixedCost<CostSlotTemplateArgument>( 1 ),
            GetLowerBoundAndCountByFixedCost<CostSlotTemplateArgument>( 3 ),
            GetLowerBoundAndCountByFixedCost<CostSlotTemplateArgument>( 4 ) };

    if ( !std::ranges::is_sorted( m_EffectiveEchos, []( auto&& EchoA, auto&& EchoB ) { return EchoA.Cost > EchoB.Cost; } ) )
    {
        spdlog::critical( "{}: Echos is not sorted by cost", SL.function_name( ) );
        return;
    }

    if ( 2000 < m_EffectiveEchos.size( ) )
    {
        spdlog::critical( "{}: Too much echoes", SL.function_name( ) );
        return;
    }

    GARuntimeReport::DetailReportQueue& DetailReport = m_GAReport.DetailReports[ GAReportIndex ];

    auto& MinHeapMaxCombinationsSet = DetailReport.QueueSet;
    auto& MinHeapMaxCombinations    = DetailReport.Queue;

    {
        std::lock_guard Lock( DetailReport.ReportLock );
        MinHeapMaxCombinationsSet.clear( );
        while ( !MinHeapMaxCombinations.empty( ) )
            MinHeapMaxCombinations.pop( );
        for ( int i = 0; i < ResultLength; ++i )
            MinHeapMaxCombinations.push( { .Value = std::numeric_limits<FloatTy>::lowest( ), .SlotCount = SlotCount } );
    }

    using RNG = XoshiroCpp::Xoshiro256Plus;
    RNG random { std::random_device { }( ) };

    std::uniform_real_distribution<FloatTy> mutation_dist( 0, 1 );
    std::uniform_real_distribution<FloatTy> crossover_dist( 0, 1 );

    std::vector<std::array<int, SlotCount>> IndicesPermutations = GeneratePermutations<SlotCount>( );
    std::vector<std::array<int, SlotCount>> Population( m_PopulationSize );

    const int AvailableFourCount  = std::ranges::count_if( m_EffectiveEchos, []( const auto& Echo ) { return Echo.Cost == 4; } );
    const int AvailableThreeCount = std::ranges::count_if( m_EffectiveEchos, []( const auto& Echo ) { return Echo.Cost == 3; } );
    const int AvailableOneCount   = std::ranges::count_if( m_EffectiveEchos, []( const auto& Echo ) { return Echo.Cost == 1; } );

    if ( CountByFixedCost[ eCost4 ] > AvailableFourCount
         || CountByFixedCost[ eCost3 ] > AvailableThreeCount
         || CountByFixedCost[ eCost1 ] > AvailableOneCount )
    {
        spdlog::critical( "{}: Not enough echoes of specified costs", SL.function_name( ) );
        return;
    }

    std::vector<int> AvailableEchoIndices( m_EffectiveEchos.size( ) );
    std::ranges::iota( AvailableEchoIndices, 0 );

    std::array<
        decltype( std::ranges::subrange( AvailableEchoIndices.begin( ), AvailableEchoIndices.end( ) ) ),
        eMaxCostIndex>
        EchoIndicesSubrangeByCost;
    EchoIndicesSubrangeByCost[ eCost4 ] = { AvailableEchoIndices.begin( ), AvailableEchoIndices.begin( ) + AvailableFourCount };
    EchoIndicesSubrangeByCost[ eCost3 ] = { AvailableEchoIndices.begin( ) + AvailableFourCount, AvailableEchoIndices.begin( ) + AvailableFourCount + AvailableThreeCount };
    EchoIndicesSubrangeByCost[ eCost1 ] = { AvailableEchoIndices.begin( ) + AvailableFourCount + AvailableThreeCount, AvailableEchoIndices.end( ) };

    {
        const auto Random4Cost = [ &EchoIndicesSubrangeByCost, &random ]( auto&& Count ) {
            std::ranges::shuffle( EchoIndicesSubrangeByCost[ eCost4 ], random );
            auto Result = std::ranges::subrange( EchoIndicesSubrangeByCost[ eCost4 ].begin( ), EchoIndicesSubrangeByCost[ eCost4 ].begin( ) + Count );
            std::ranges::sort( Result );
            return Result;
        };
        const auto Random3Cost = [ &EchoIndicesSubrangeByCost, &random ]( auto&& Count ) {
            std::ranges::shuffle( EchoIndicesSubrangeByCost[ eCost3 ], random );
            auto Result = std::ranges::subrange( EchoIndicesSubrangeByCost[ eCost3 ].begin( ), EchoIndicesSubrangeByCost[ eCost3 ].begin( ) + Count );
            std::ranges::sort( Result );
            return Result;
        };
        const auto Random1Cost = [ &EchoIndicesSubrangeByCost, &random ]( auto&& Count ) {
            std::ranges::shuffle( EchoIndicesSubrangeByCost[ eCost1 ], random );
            auto Result = std::ranges::subrange( EchoIndicesSubrangeByCost[ eCost1 ].begin( ), EchoIndicesSubrangeByCost[ eCost1 ].begin( ) + Count );
            std::ranges::sort( Result );
            return Result;
        };

        /*
         *
         * Initialize population with random echoes
         *
         * */
        for ( auto& Individual : Population )
        {
            auto CopyBegin = Individual.begin( );

            auto RandSubrange = Random4Cost( CountByFixedCost[ eCost4 ] );
            CopyBegin         = std::ranges::copy( RandSubrange, CopyBegin ).out;
            RandSubrange      = Random3Cost( CountByFixedCost[ eCost3 ] );
            CopyBegin         = std::ranges::copy( RandSubrange, CopyBegin ).out;
            RandSubrange      = Random1Cost( CountByFixedCost[ eCost1 ] );
            /*****************/ std::ranges::copy( RandSubrange, CopyBegin );
        }
    }

    int MutationProbabilityChangeCount = 0;

    auto& MutationProbability = m_GAReport.MutationProb[ GAReportIndex ] = 0;
    auto& OptimalValue = m_GAReport.OptimalValue[ GAReportIndex ] = std::numeric_limits<FloatTy>::lowest( );

    // Pre-allocated memories
    std::unordered_map<uint64_t, FloatTy>    StatsCache;
    std::vector<EffectiveStats>              EffectiveStatsPlaceHolder( SlotCount );
    std::unordered_set<uint64_t>             ExistentialCache;
    std::vector<std::pair<FloatTy, int64_t>> Fitness_Index( m_PopulationSize );
    std::vector<std::pair<FloatTy, int64_t>> TopFitness_Index( m_ReproduceSize );
    std::array<PreAllocatedBuffer<
                   int, std::ranges::max( CountByFixedCost ) * 2>,
               eMaxCostIndex>
        ReproduceIndicesBuffer;

    Stopwatch SW;

    while ( !StopToken.stop_requested( ) )
    {
        /*
         *
         * Selection
         *
         * */
        ExistentialCache.clear( );
        TopFitness_Index.clear( );
        for ( int i = 0, top_count = 0; i < m_PopulationSize; ++i )
        {
            auto& CurrentCombination = Population[ i ];

            uint64_t CombinationID = 0;
            for ( int j = 0; j < SlotCount; ++j )
            {
                CombinationID |= (uint64_t) CurrentCombination[ j ] << j * IndexBitsShift;
            }

            // Calculated in the current round
            if ( ExistentialCache.contains( CombinationID ) )
            {
                continue;
            }

            FloatTy Fitness = 0;

            const auto StatsCacheIt = StatsCache.find( CombinationID );
            if ( StatsCacheIt != StatsCache.end( ) )
            {
                // Use cached result
                Fitness = StatsCacheIt->second;
            } else
            {
                std::ranges::copy( CurrentCombination
                                       | std::views::transform( [ this ]( int EchoIndex ) {
                                             return m_EffectiveEchos[ EchoIndex ];
                                         } ),
                                   EffectiveStatsPlaceHolder.begin( ) );

                // First time calculating
                const auto FinalStat = CalculateCombinationalStat<ETy>( EffectiveStatsPlaceHolder, CommonStats );

                Fitness = Constraints( FinalStat )
                    ? FinalStat.ExpectedDamage( BaseAttack, OptimizeMultiplierConfig )
                    : 0;

                StatsCache.insert( StatsCacheIt, { CombinationID, Fitness } );
            }

            if ( top_count < m_ReproduceSize )
            {
                ++top_count;
                TopFitness_Index.emplace_back( Fitness, i );
                std::ranges::push_heap( TopFitness_Index, std::greater<> { } );
            } else if ( TopFitness_Index[ 0 ].first < Fitness )
            {
                std::ranges::pop_heap( TopFitness_Index, std::greater<> { } );
                TopFitness_Index[ m_ReproduceSize - 1 ] = std::make_pair( Fitness, i );
                std::ranges::push_heap( TopFitness_Index, std::greater<> { } );
            }
        }

        std::ranges::sort_heap( TopFitness_Index, std::greater<> { } );

        if ( OptimalValue >= TopFitness_Index.front( ).first )
        {
            MutationProbability += 0.01f;

            // Finish
            if ( MutationProbability >= 1 )
            {
                break;
            }
        } else
        {
            OptimalValue = TopFitness_Index.front( ).first;
        }

        // Store best result
        for ( const auto& TPFI : TopFitness_Index )
        {
            if ( MinHeapMaxCombinations.top( ).Value < TPFI.first )
            {
                CombinationRecord NewRecord { .Value = TPFI.first, .SlotCount = SlotCount };
                for ( int i = 0; i < SlotCount; ++i )
                    NewRecord.SetAt( Population[ TPFI.second ][ i ], i );

                // need to sort by index
                if ( !MinHeapMaxCombinationsSet.contains( NewRecord.CombinationData ) )
                {
                    std::lock_guard Lock( DetailReport.ReportLock );

                    MinHeapMaxCombinationsSet.erase( MinHeapMaxCombinations.top( ).CombinationData );
                    MinHeapMaxCombinationsSet.insert( NewRecord.CombinationData );
                    MinHeapMaxCombinations.pop( );
                    MinHeapMaxCombinations.push( NewRecord );

                    spdlog::info( "New record - {}", NewRecord.ToString( ) );
                }
            } else
            {
                break;
            }
        }

        /*
         *
         * Grouping elites
         *
         * */
        // Make sure range won't overlap by sorting by index
        std::ranges::sort( TopFitness_Index, []( auto& A, auto& B ) {
            return A.second < B.second;
        } );

        // Write to first m_ReproduceSize th element in population
        std::ranges::copy(
            TopFitness_Index
                | std::views::transform( [ &Population ]( auto& FI ) { return Population[ FI.second ]; } ),
            Population.begin( ) );

        for ( auto& Individual : std::ranges::subrange( Population.begin( ) + TopFitness_Index.size( ), Population.end( ) ) )
        {
            /*
             *
             * Crossover
             *
             * */
            const auto FirstPickIndex  = static_cast<int>( crossover_dist( random ) * ( m_ReproduceSize - 1 ) );
            const auto SecondPickIndex = static_cast<int>( crossover_dist( random ) * ( m_ReproduceSize - 1 ) );

            auto FirstParentIter  = Population[ FirstPickIndex ].begin( );
            auto SecondParentIter = Population[ SecondPickIndex ].begin( );

            // Merge to parent set, eliminating duplicates
            for ( int CostIndex = eMaxCostIndex - 1; CostIndex >= 0; --CostIndex )
            {
                auto& CurrentIndices = ReproduceIndicesBuffer[ CostIndex ];
                CurrentIndices.Clear( );

                const auto Count = CountByFixedCost[ CostIndex ];

                int ParentAIndex = 0, ParentBIndex = 0;
                while ( ParentAIndex < Count && ParentBIndex < Count )
                {
                    if ( *FirstParentIter < *SecondParentIter )
                    {
                        CurrentIndices.PushBack( *FirstParentIter );
                        ++FirstParentIter;
                        ++ParentAIndex;
                    } else if ( *FirstParentIter > *SecondParentIter )
                    {
                        CurrentIndices.PushBack( *SecondParentIter );
                        ++SecondParentIter;
                        ++ParentBIndex;
                    } else
                    {
                        // Repeat
                        CurrentIndices.PushBack( *FirstParentIter );
                        ++FirstParentIter;
                        ++ParentAIndex;
                        ++SecondParentIter;
                        ++ParentBIndex;
                    }
                }

                while ( ParentAIndex < Count )
                {
                    CurrentIndices.PushBack( *FirstParentIter );
                    ++FirstParentIter;
                    ++ParentAIndex;
                }

                while ( ParentBIndex < Count )
                {
                    CurrentIndices.PushBack( *SecondParentIter );
                    ++SecondParentIter;
                    ++ParentBIndex;
                }
            }

            int InsertIndex = 0;
            for ( int CostIndex = eMaxCostIndex - 1; CostIndex >= 0; --CostIndex )
            {
                const auto& Indices = IndicesPermutations[ random( ) % MaxPermutation ];
                const auto& Set     = ReproduceIndicesBuffer[ CostIndex ];
                const auto  Count   = CountByFixedCost[ CostIndex ];
                for ( int i = 0, ScanIndex = -1; i < Count; ++i )
                {
                    while ( Indices[ ++ScanIndex ] >= Set.Size )
                        ;
                    Individual[ InsertIndex + i ] = Set[ Indices[ ScanIndex ] ];
                    std::push_heap( Individual.begin( ) + InsertIndex, Individual.begin( ) + InsertIndex + i + 1 );
                }

                std::sort_heap( Individual.begin( ) + InsertIndex, Individual.begin( ) + InsertIndex + Count );
                InsertIndex += Count;
            }

            /*
             *
             * Mutation
             *
             * */
            int MutationTime = 0;
            while ( ++MutationTime <= SlotCount && mutation_dist( random ) < MutationProbability )
            {
                const int MutationAtCost = CostToIndex( GetCostAt<CostSlotTemplateArgument>( random( ) % SlotCount ) );

                auto [ MutationStart, MutationCount ] = LowerBoundAndCountByFixedCost[ MutationAtCost ];

                const auto  MutationAtIndex    = MutationStart + int( random( ) % MutationCount );
                const auto& AvailableEchoRange = EchoIndicesSubrangeByCost[ MutationAtCost ];

                int MaxTries     = 10;
                int NewEchoIndex = 0;
                do
                {
                    NewEchoIndex = AvailableEchoRange[ random( ) % AvailableEchoRange.size( ) ];
                } while ( --MaxTries && std::ranges::contains( Individual.data( ) + MutationStart, Individual.data( ) + MutationStart + MutationCount, NewEchoIndex ) );
                if ( MaxTries == 0 ) break;

                // Basic one round bubble sort
                if ( Individual[ MutationAtIndex ] > NewEchoIndex )
                {
                    Individual[ MutationAtIndex ] = NewEchoIndex;
                    for ( int i = MutationAtIndex; i > MutationStart; --i )
                    {
                        if ( Individual[ i ] < Individual[ i - 1 ] )
                            std::swap( Individual[ i ], Individual[ i - 1 ] );
                    }
                } else
                {
                    Individual[ MutationAtIndex ] = NewEchoIndex;
                    for ( int i = MutationAtIndex; i < MutationStart + MutationCount - 1; ++i )
                    {
                        if ( Individual[ i ] > Individual[ i + 1 ] )
                            std::swap( Individual[ i ], Individual[ i + 1 ] );
                    }
                }
            }
        }
    }

    spdlog::info( "Final rand: {}", random( ) );

    // For visualizer
    MutationProbability = -1;
}

template <ElementType ETy>
inline void
WuWaGA::Run( FloatTy BaseAttack, const EffectiveStats& CommonStats, const SkillMultiplierConfig* OptimizeMultiplierConfig, const EchoConstraint& Constraints )
{
    m_Threads.clear( );

    if ( m_Echos == nullptr )
    {
        spdlog::warn( "Empty echo set to optimize" );
        return;
    }

    m_EffectiveEchos =
        ( *m_Echos )
        | std::views::transform( ToEffectiveStats<ETy> )
        | std::ranges::to<std::vector>( );

    assert( m_EffectiveEchos.size( ) == m_Echos->size( ) );

    // clang-format off
    m_Threads.emplace_back( std::make_unique<std::jthread>( std::bind(&WuWaGA::Run<ETy, 4, 4, 4, 0, 0>, this, std::placeholders::_1,  0, BaseAttack, CommonStats, OptimizeMultiplierConfig, Constraints ) ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>( std::bind(&WuWaGA::Run<ETy, 4, 4, 3, 1, 0>, this, std::placeholders::_1,  1, BaseAttack, CommonStats, OptimizeMultiplierConfig, Constraints ) ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>( std::bind(&WuWaGA::Run<ETy, 3, 3, 3, 3, 0>, this, std::placeholders::_1,  2, BaseAttack, CommonStats, OptimizeMultiplierConfig, Constraints ) ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>( std::bind(&WuWaGA::Run<ETy, 4, 4, 1, 1, 1>, this, std::placeholders::_1,  3, BaseAttack, CommonStats, OptimizeMultiplierConfig, Constraints ) ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>( std::bind(&WuWaGA::Run<ETy, 4, 1, 1, 1, 1>, this, std::placeholders::_1,  4, BaseAttack, CommonStats, OptimizeMultiplierConfig, Constraints ) ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>( std::bind(&WuWaGA::Run<ETy, 4, 3, 3, 1, 1>, this, std::placeholders::_1,  5, BaseAttack, CommonStats, OptimizeMultiplierConfig, Constraints ) ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>( std::bind(&WuWaGA::Run<ETy, 4, 3, 1, 1, 1>, this, std::placeholders::_1,  6, BaseAttack, CommonStats, OptimizeMultiplierConfig, Constraints ) ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>( std::bind(&WuWaGA::Run<ETy, 3, 1, 1, 1, 1>, this, std::placeholders::_1,  7, BaseAttack, CommonStats, OptimizeMultiplierConfig, Constraints ) ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>( std::bind(&WuWaGA::Run<ETy, 3, 3, 1, 1, 1>, this, std::placeholders::_1,  8, BaseAttack, CommonStats, OptimizeMultiplierConfig, Constraints ) ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>( std::bind(&WuWaGA::Run<ETy, 3, 3, 3, 1, 1>, this, std::placeholders::_1,  9, BaseAttack, CommonStats, OptimizeMultiplierConfig, Constraints ) ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>( std::bind(&WuWaGA::Run<ETy, 1, 1, 1, 1, 1>, this, std::placeholders::_1, 10, BaseAttack, CommonStats, OptimizeMultiplierConfig, Constraints ) ) );
    // clang-format on
}

inline bool
WuWaGA::IsRunning( ) const
{
    return std::ranges::any_of( m_GAReport.MutationProb, []( auto Prob ) { return Prob > 0; } );
}

// 2.5 seconds
// 2.8 seconds, add four more stats