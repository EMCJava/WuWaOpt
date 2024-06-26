//
// Created by EMCJava on 6/16/2024.
//

#include "WuWaGa.hpp"
#include "Random.hpp"

#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <ranges>
#include <random>

enum ECostToIndex {
    eCost1 = 0,
    eCost3 = 1,
    eCost4 = 2,
    eMaxCostIndex
};

constexpr ECostToIndex
CostToIndex( int Cost )
{
    __assume( Cost == 1 || Cost == 3 || Cost == 4 );

    if ( Cost == 1 ) return eCost1;
    if ( Cost == 3 ) return eCost3;
    if ( Cost == 4 ) return eCost4;

    std::unreachable( );
}

CostSlotTemplate constexpr int
GetSlotCount( )
{
    constexpr std::array<int, 5> Costs { CostSlotTemplateArgument };
    for ( int i = 0; i < 5; ++i )
        if ( Costs[ i ] == 0 ) return i;
    return 5;
}

CostSlotTemplate constexpr int
GetCountByFixedCost( int Cost )
{
    constexpr std::array<int, 5> Costs { CostSlotTemplateArgument };

    int Result = 0;
    for ( int i = 0; i < 5; ++i )
        if ( Costs[ i ] == Cost ) ++Result;
    return Result;
}

CostSlotTemplate constexpr std::pair<int, int>
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

CostSlotTemplate constexpr int
GetCostAt( int Index )
{
    constexpr std::array<int, 5> Costs { CostSlotTemplateArgument };
    return Costs[ Index ];
}

constexpr auto
Factorial( auto X )
{
    if ( 0 == X )
        return 1;
    return X * Factorial( X - 1 );
}

template <int SlotCount>
constexpr auto
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

CostSlotTemplate void
WuWaGA::Run( int GAReportIndex, FloatTy BaseAttack )
{
    static constexpr int MaxEchoCount     = 2000;
    static constexpr int IndexBitsShift   = 11;
    const auto           ReproduceSizeBy5 = m_ReproduceSize / 5;

    constexpr int  SlotCount      = GetSlotCount<CostSlotTemplateArgument>( );
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
        throw std::runtime_error( "GeneticAlgorithm: Echos is not sorted by cost" );
    }

    if ( 2000 < m_EffectiveEchos.size( ) )
    {
        throw std::runtime_error( "GeneticAlgorithm: Too much echos" );
    }

    GARuntimeReport::DetailReportQueue& DetailReport = m_GAReport.DetailReports[ GAReportIndex ];

    auto& MinHeapMaxCombinationsSet = DetailReport.QueueSet;
    MinHeapMaxCombinationsSet.clear( );
    auto& MinHeapMaxCombinations = DetailReport.Queue;
    while ( !MinHeapMaxCombinations.empty( ) )
        MinHeapMaxCombinations.pop( );
    for ( int i = 0; i < ResultLength; ++i )
        MinHeapMaxCombinations.push( { .Value = std::numeric_limits<FloatTy>::min( ), .SlotCount = SlotCount } );

    using RNG = XoshiroCpp::Xoshiro256Plus;
    RNG random { std::random_device { }( ) };

    std::uniform_real_distribution<FloatTy> mutation_dist( 0, 1 );
    std::uniform_real_distribution<FloatTy> crossover_dist( 0, 1 );

    std::vector<std::array<int, SlotCount>> IndicesPermutations = GeneratePermutations<SlotCount>( );
    std::vector<std::array<int, SlotCount>> Population( m_PopulationSize );

    // Echos is pre sorted by cost
    const auto EchoCounts = m_EffectiveEchos
        | std::views::chunk_by( []( auto& A, auto& B ) { return A.Cost == B.Cost; } )
        | std::views::transform( []( const auto& Range ) -> int { return std::ranges::distance( Range ); } )
        | std::ranges::to<std::vector>( );
    const int AvailableFourCount  = EchoCounts[ 0 ];
    const int AvailableThreeCount = EchoCounts[ 1 ];
    const int AvailableOneCount   = EchoCounts[ 2 ];

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
         * Initialize population with random echos
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

    auto& MutationProbability = m_GAReport.MutationProb[ GAReportIndex ];
    auto& OptimalValue        = m_GAReport.OptimalValue[ GAReportIndex ];
    OptimalValue              = std::numeric_limits<FloatTy>::lowest( );

    m_GAReport.ParentPickCount[ GAReportIndex ].resize( m_ReproduceSize );


    // Pre-allocated memories
    std::unordered_map<uint64_t, FloatTy>    StatsCache;
    std::vector<std::pair<FloatTy, int64_t>> Fitness_Index( m_PopulationSize );
    std::vector<std::pair<FloatTy, int64_t>> TopFitness_Index( m_ReproduceSize );
    std::array<PreAllocatedBuffer<
                   int, std::ranges::max( CountByFixedCost ) * 2>,
               eMaxCostIndex>
        ReproduceIndicesBuffer;

    Stopwatch SW;

    while ( true )
    {
        /*
         *
         * Selection
         *
         * */
        StatsCache.clear( );
        TopFitness_Index.clear( );
        for ( int i = 0, top_count = 0; i < m_PopulationSize; ++i )
        {
            uint64_t CombinationID = 0;
            for ( int j = 0; j < SlotCount; ++j )
            {
                CombinationID |= (uint64_t) Population[ i ][ j ] << j * IndexBitsShift;
            }

            if ( StatsCache.contains( CombinationID ) )
            {
                continue;
            }

            const auto Fitness = std::ranges::fold_left(
                                     Population[ i ],
                                     EffectiveStats { }, [ this ]( auto&& Stat, int EchoIndex ) {
                                         return Stat + m_EffectiveEchos[ EchoIndex ];
                                     } )
                                     .ExpectedDamage( BaseAttack );

            StatsCache.insert( { CombinationID, Fitness } );

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
            MutationProbability += 0.0001f;

            if ( ++MutationProbabilityChangeCount % 1000 == 0 )
            {
                const auto NewSeed = XoshiroCpp::SplitMix64 { std::random_device { }( ) }.generateSeedSequence<4>( );
                random.deserialize( NewSeed );
                spdlog::info( "No Max Fitness: MutationProbability({}) {}", MutationProbability, MutationProbabilityChangeCount );
            }

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

        for ( auto& Individual : std::ranges::subrange( Population.begin( ) + TopFitness_Index.size( ), Population.end( ) ) )
        {
            /*
             *
             * Crossover
             *
             * */
            const auto FirstPickIndex  = static_cast<int>( crossover_dist( random ) * ( TopFitness_Index.size( ) - 1 ) );
            const auto SecondPickIndex = static_cast<int>( crossover_dist( random ) * ( TopFitness_Index.size( ) - 1 ) );

            m_GAReport.ParentPickCount[ GAReportIndex ][ FirstPickIndex ]++;
            m_GAReport.ParentPickCount[ GAReportIndex ][ SecondPickIndex ]++;

            auto FirstParentIter  = Population[ TopFitness_Index[ FirstPickIndex ].second ].begin( );
            auto SecondParentIter = Population[ TopFitness_Index[ SecondPickIndex ].second ].begin( );

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

                int NewEchoIndex = 0;
                do
                {
                    NewEchoIndex = AvailableEchoRange[ random( ) % AvailableEchoRange.size( ) ];
                } while ( std::ranges::contains( Individual.data( ) + MutationStart, Individual.data( ) + MutationStart + MutationCount, NewEchoIndex ) );

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

        // Make sure range won't overlap by sorting by index
        std::ranges::sort( TopFitness_Index, []( auto& A, auto& B ) {
            return A.second < B.second;
        } );

        // Write to first m_ReproduceSize th element in population
        std::ranges::copy(
            TopFitness_Index
                | std::views::transform( [ &Population ]( auto& FI ) { return Population[ FI.second ]; } ),
            Population.begin( ) );
    }

    spdlog::info( "Final rand: {}", random( ) );

    // For visualizer
    MutationProbability = -1;
}

void
WuWaGA::Run( )
{
    m_Threads.clear( );

    m_EffectiveEchos =
        m_Echos
        | std::views::transform( ToEffectiveStats<eFireDamagePercentage, eAutoAttackDamagePercentage> )
        | std::ranges::to<std::vector>( );

    // clang-format off
    m_Threads.emplace_back( std::make_unique<std::jthread>([&](){ Run< 4, 4, 4, 0, 0>( 0 , 500 );} ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>([&](){ Run< 4, 4, 3, 1, 0>( 1 , 500 );} ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>([&](){ Run< 3, 3, 3, 3, 0>( 2 , 500 );} ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>([&](){ Run< 4, 4, 1, 1, 1>( 3 , 500 );} ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>([&](){ Run< 4, 1, 1, 1, 1>( 4 , 500 );} ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>([&](){ Run< 4, 3, 3, 1, 1>( 5 , 500 );} ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>([&](){ Run< 4, 3, 1, 1, 1>( 6 , 500 );} ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>([&](){ Run< 3, 1, 1, 1, 1>( 7 , 500 );} ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>([&](){ Run< 3, 3, 1, 1, 1>( 8 , 500 );} ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>([&](){ Run< 3, 3, 3, 1, 1>( 9 , 500 );} ) );
    m_Threads.emplace_back( std::make_unique<std::jthread>([&](){ Run< 1, 1, 1, 1, 1>( 10, 500 );} ) );
    // clang-format on
}
