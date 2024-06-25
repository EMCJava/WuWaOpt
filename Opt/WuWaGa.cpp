//
// Created by EMCJava on 6/16/2024.
//

#include "WuWaGa.hpp"
#include "Random.hpp"

#include <stdexcept>
#include <algorithm>
#include <vector>
#include <ranges>
#include <random>

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
                LowerBound = i;
            else
                ++Count;
        }
    }
    return { LowerBound, Count };
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

CostSlotTemplate void
WuWaGA::Run( int GAReportIndex, FloatTy BaseAttack )
{
    static constexpr int MaxEchoCount     = 2000;
    const auto           ReproduceSizeBy5 = m_ReproduceSize / 5;

    constexpr int  SlotCount      = GetSlotCount<CostSlotTemplateArgument>( );
    constexpr auto MaxPermutation = Factorial( SlotCount );

    if ( !std::ranges::is_sorted( m_Echos, []( auto&& EchoA, auto&& EchoB ) { return EchoA.Cost > EchoB.Cost; } ) )
    {
        throw std::runtime_error( "GeneticAlgorithm: m_Echos is not sorted by cost" );
    }

    if ( 2000 < m_Echos.size( ) )
    {
        throw std::runtime_error( "GeneticAlgorithm: m_Echos is too much" );
    }

    GARuntimeReport::DetailReportQueue& DetailReport = m_GAReport.DetailReports[ GAReportIndex ];

    auto& MinHeapMaxCombinationsSet = DetailReport.QueueSet;
    MinHeapMaxCombinationsSet.clear( );
    auto& MinHeapMaxCombinations = DetailReport.Queue;
    while ( !MinHeapMaxCombinations.empty( ) )
        MinHeapMaxCombinations.pop( );
    for ( int i = 0; i < m_ResultLength; ++i )
        MinHeapMaxCombinations.push( { .Value = std::numeric_limits<FloatTy>::min( ), .SlotCount = SlotCount } );

    using RNG = XoshiroCpp::Xoshiro256Plus;
    RNG random { std::random_device { }( ) };

    std::uniform_real_distribution<FloatTy> real_dist( 0, 1 );
    std::exponential_distribution<FloatTy>  expo_dist( 1.0 );

    constexpr std::array<int, 5> CountByFixedCost {
        0,
        GetCountByFixedCost<CostSlotTemplateArgument>( 1 ),
        0,
        GetCountByFixedCost<CostSlotTemplateArgument>( 3 ),
        GetCountByFixedCost<CostSlotTemplateArgument>( 4 ) };

    constexpr std::array<std::pair<int, int>, 5> LowerBoundAndCountByFixedCost {
        std::pair<int, int> {0, 0},
        GetLowerBoundAndCountByFixedCost<CostSlotTemplateArgument>( 1 ),
        std::pair<int, int> {0, 0},
        GetLowerBoundAndCountByFixedCost<CostSlotTemplateArgument>( 3 ),
        GetLowerBoundAndCountByFixedCost<CostSlotTemplateArgument>( 4 )
    };

    using EquipmentIndex = int;

    auto PopulationBuffer = std::make_unique<EquipmentIndex[]>( SlotCount * m_PopulationSize );

    std::vector<std::ranges::subrange<EquipmentIndex*, EquipmentIndex*>> Population( m_PopulationSize );
    for ( int i = 0; i < m_PopulationSize; ++i )
    {
        Population[ i ] = std::ranges::subrange( PopulationBuffer.get( ) + i * SlotCount, PopulationBuffer.get( ) + ( i + 1 ) * SlotCount );
    }

    const auto EchoCounts = m_Echos
        | std::views::chunk_by( []( auto& A, auto& B ) { return A.Cost == B.Cost; } )
        | std::views::transform( []( const auto& Range ) -> int { return std::ranges::distance( Range ); } )
        | std::ranges::to<std::vector>( );
    const int FourCount  = EchoCounts[ 0 ];
    const int ThreeCount = EchoCounts[ 1 ];
    const int OneCount   = EchoCounts[ 2 ];

    std::size_t IndicesPermutationsBufferSize = MaxPermutation * SlotCount;
    auto        IndicesPermutationsBuffer     = std::make_unique<int[]>( IndicesPermutationsBufferSize );

    std::vector<std::ranges::subrange<int*, int*>> IndicesPermutations( MaxPermutation );
    {
        auto* PermutationsBufferBegin = IndicesPermutationsBuffer.get( );

        std::vector<int> Permutation( SlotCount );
        std::ranges::iota( Permutation, 0 );
        do
        {
            std::memcpy( PermutationsBufferBegin, Permutation.data( ), SlotCount * sizeof( int ) );
            PermutationsBufferBegin += SlotCount;
        } while ( std::ranges::next_permutation( Permutation ).found );

        for ( int i = 0; i < MaxPermutation; ++i )
        {
            IndicesPermutations[ i ] = std::ranges::subrange( IndicesPermutationsBuffer.get( ) + i * SlotCount, IndicesPermutationsBuffer.get( ) + ( i + 1 ) * SlotCount );
        }
    }

    std::vector<int> EchoIndices( m_Echos.size( ) );
    std::ranges::iota( EchoIndices, 0 );

    std::array<
        decltype( std::ranges::subrange( EchoIndices.begin( ), EchoIndices.end( ) ) ),
        5>
        EchoIndicesSubrangeByCost;
    EchoIndicesSubrangeByCost[ 4 ] = { EchoIndices.begin( ), EchoIndices.begin( ) + FourCount };
    EchoIndicesSubrangeByCost[ 3 ] = { EchoIndices.begin( ) + FourCount, EchoIndices.begin( ) + FourCount + ThreeCount };
    EchoIndicesSubrangeByCost[ 1 ] = { EchoIndices.begin( ) + FourCount + ThreeCount, EchoIndices.end( ) };

    const auto Random4Cost = [ &EchoIndicesSubrangeByCost, &random ]( auto&& Count ) {
        std::ranges::shuffle( EchoIndicesSubrangeByCost[ 4 ], random );
        return std::ranges::subrange( EchoIndicesSubrangeByCost[ 4 ].begin( ), EchoIndicesSubrangeByCost[ 4 ].begin( ) + Count );
    };
    const auto Random3Cost = [ &EchoIndicesSubrangeByCost, &random ]( auto&& Count ) {
        std::ranges::shuffle( EchoIndicesSubrangeByCost[ 3 ], random );
        return std::ranges::subrange( EchoIndicesSubrangeByCost[ 3 ].begin( ), EchoIndicesSubrangeByCost[ 3 ].begin( ) + Count );
    };
    const auto Random1Cost = [ &EchoIndicesSubrangeByCost, &random ]( auto&& Count ) {
        std::ranges::shuffle( EchoIndicesSubrangeByCost[ 1 ], random );
        return std::ranges::subrange( EchoIndicesSubrangeByCost[ 1 ].begin( ), EchoIndicesSubrangeByCost[ 1 ].begin( ) + Count );
    };
    /*
     *
     * Initialize population with random m_Echos
     *
     * */
    for ( auto& Individual : Population )
    {
        auto CopyBegin = Individual.begin( );

        auto RandSubrange = Random4Cost( CountByFixedCost[ 4 ] );
        CopyBegin         = std::ranges::copy( RandSubrange, CopyBegin ).out;
        RandSubrange      = Random3Cost( CountByFixedCost[ 3 ] );
        CopyBegin         = std::ranges::copy( RandSubrange, CopyBegin ).out;
        RandSubrange      = Random1Cost( CountByFixedCost[ 1 ] );
        /*****************/ std::ranges::copy( RandSubrange, CopyBegin );
    }

    int MutationProbabilityChangeCount = 0;

    auto& MutationProbability = m_GAReport.MutationProb[ GAReportIndex ];
    auto& OptimalValue        = m_GAReport.OptimalValue[ GAReportIndex ];
    OptimalValue              = std::numeric_limits<FloatTy>::lowest( );

    m_GAReport.ParentPickCount[ GAReportIndex ].resize( m_ReproduceSize );

    Stopwatch SW;

    // Pre allocated memories
    std::vector<std::pair<FloatTy, int64_t>> Fitness_Index( m_PopulationSize );
    std::vector<std::pair<FloatTy, int64_t>> TopPopulationFitness_Index( m_ReproduceSize );
    std::array<BoolArray<MaxEchoCount>, 5>   IndexSets;
    std::array<std::vector<int>, 5>          ReproduceIndices;

    while ( true )
    {
        /*
         *
         * Selection
         *
         * */
        memset( TopPopulationFitness_Index.data( ), 0, sizeof( TopPopulationFitness_Index ) );
        for ( int i = 0, top_count = 0; i < m_PopulationSize; ++i )
        {
            const auto Fitness = std::ranges::fold_left(
                                     Population[ i ],
                                     EffectiveStats { }, [ this ]( auto&& Stat, int EchoIndex ) {
                                         return Stat + m_Echos[ EchoIndex ];
                                     } )
                                     .ExpectedDamage( BaseAttack );

            if ( top_count < m_ReproduceSize ) [[unlikely]]
            {
                TopPopulationFitness_Index[ top_count++ ] = std::make_pair( Fitness, i );
                std::ranges::push_heap( TopPopulationFitness_Index.begin( ), TopPopulationFitness_Index.begin( ) + top_count, std::greater<> { } );
            } else if ( TopPopulationFitness_Index[ 0 ].first < Fitness )
            {
                std::ranges::pop_heap( TopPopulationFitness_Index, std::greater<> { } );
                TopPopulationFitness_Index[ m_ReproduceSize - 1 ] = std::make_pair( Fitness, i );
                std::ranges::push_heap( TopPopulationFitness_Index, std::greater<> { } );
            }
        }

        std::ranges::sort_heap( TopPopulationFitness_Index, std::greater<> { } );

        if ( OptimalValue >= TopPopulationFitness_Index.front( ).first )
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
            OptimalValue = TopPopulationFitness_Index.front( ).first;
        }

        // Store best result
        for ( const auto& TPFI : TopPopulationFitness_Index )
        {
            if ( MinHeapMaxCombinations.top( ).Value < TPFI.first )
            {
                CombinationRecord NewRecord { .Value = TPFI.first, .SlotCount = SlotCount };

                std::vector<int> SortedIndices( SlotCount );
                std::ranges::copy( Population[ TPFI.second ], SortedIndices.begin( ) );
                std::ranges::sort( SortedIndices );
                for ( int i = 0; i < SlotCount; ++i )
                    NewRecord.SetAt( SortedIndices[ i ], i );

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

        for ( auto& Individual : std::ranges::subrange( Population.begin( ) + m_ReproduceSize, Population.end( ) ) )
        {
            /*
             *
             * Crossover
             *
             * */
            const auto FirstPickIndex  = std::abs( static_cast<int>( expo_dist( random ) * ReproduceSizeBy5 ) % m_ReproduceSize );
            const auto SecondPickIndex = std::abs( static_cast<int>( expo_dist( random ) * ReproduceSizeBy5 ) % m_ReproduceSize );

            m_GAReport.ParentPickCount[ GAReportIndex ][ FirstPickIndex ]++;
            m_GAReport.ParentPickCount[ GAReportIndex ][ SecondPickIndex ]++;

            std::array<int, 2> ParentPopulationIndices;
            ParentPopulationIndices[ 0 ] = TopPopulationFitness_Index[ FirstPickIndex ].second;
            ParentPopulationIndices[ 1 ] = TopPopulationFitness_Index[ SecondPickIndex ].second;

            IndexSets[ 4 ].Reset( );
            IndexSets[ 3 ].Reset( );
            IndexSets[ 1 ].Reset( );

            ReproduceIndices[ 4 ].clear( );
            ReproduceIndices[ 3 ].clear( );
            ReproduceIndices[ 1 ].clear( );

            for ( const auto& ParentIndex : ParentPopulationIndices )
            {
                const auto& Parent   = Population[ ParentIndex ];
                auto        ParentIt = Parent.begin( );
                for ( int i = 0; i < CountByFixedCost[ 4 ]; ++i, ++ParentIt )
                    if ( IndexSets[ 4 ].SetAt( *ParentIt ) )
                        ReproduceIndices[ 4 ].push_back( *ParentIt );
                for ( int i = 0; i < CountByFixedCost[ 3 ]; ++i, ++ParentIt )
                    if ( IndexSets[ 3 ].SetAt( *ParentIt ) )
                        ReproduceIndices[ 3 ].push_back( *ParentIt );
                for ( int i = 0; i < CountByFixedCost[ 1 ]; ++i, ++ParentIt )
                    if ( IndexSets[ 1 ].SetAt( *ParentIt ) )
                        ReproduceIndices[ 1 ].push_back( *ParentIt );
            }

            const auto AppendByIndices = []( auto& Indices, auto& Set, auto& It, int Count ) {
                for ( int i = 0, ScanIndex = -1; i < Count; ++i, ++It )
                {
                    while ( Indices[ ++ScanIndex ] >= Set.size( ) )
                        ;
                    *It = Set[ Indices[ ScanIndex ] ];
                }
            };

            auto IndividualIt = Individual.begin( );
            AppendByIndices( IndicesPermutations[ random( ) % MaxPermutation ], ReproduceIndices[ 4 ], IndividualIt, CountByFixedCost[ 4 ] );
            AppendByIndices( IndicesPermutations[ random( ) % MaxPermutation ], ReproduceIndices[ 3 ], IndividualIt, CountByFixedCost[ 3 ] );
            AppendByIndices( IndicesPermutations[ random( ) % MaxPermutation ], ReproduceIndices[ 1 ], IndividualIt, CountByFixedCost[ 1 ] );
            assert( IndividualIt == Individual.end( ) );

            /*
             *
             * Mutation
             *
             * */
            while ( false && real_dist( random ) < MutationProbability )
            {
                const auto MutationAtCost = GetCostAt<CostSlotTemplateArgument>( random( ) % SlotCount );

                auto [ MutationStart, MutationCount ] = LowerBoundAndCountByFixedCost[ MutationAtCost ];

                const auto  MutationAtIndex    = MutationStart + random( ) % MutationCount;
                const auto& AvailableEchoRange = EchoIndicesSubrangeByCost[ MutationAtCost ];

                int NewEchoIndex = 0;
                do
                {
                    NewEchoIndex = AvailableEchoRange[ random( ) % AvailableEchoRange.size( ) ];
                } while ( std::ranges::contains( Individual.data( ) + MutationStart, Individual.data( ) + MutationStart + MutationCount, NewEchoIndex ) );

                Individual[ MutationAtIndex ] = NewEchoIndex;
            }
        }

        // Make sure range won't overlap by sorting by index
        std::ranges::sort( TopPopulationFitness_Index, []( auto& A, auto& B ) {
            return A.second < B.second;
        } );

        // Write to first m_ReproduceSize th element in population
        std::ranges::copy(
            TopPopulationFitness_Index
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
