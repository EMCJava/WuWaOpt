#include <fstream>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <utility>
#include <chrono>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <set>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <imgui.h>   // necessary for ImGui::*, imgui-SFML.h doesn't include imgui.h

#include <imgui-SFML.h>   // for ImGui::SFML::* functions and SFML-specific overloads

#include <implot.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <random>

#include "Random.hpp"

#include "FullStats.hpp"

class Stopwatch
{
public:
    Stopwatch( )
        : start_time( std::chrono::high_resolution_clock::now( ) )
    { }

    ~Stopwatch( )
    {
        auto end_time = std::chrono::high_resolution_clock::now( );
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end_time - start_time ).count( );
        spdlog::info( "Elapsed time: {} seconds", duration / 1000'000.f );
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

struct StatsCache {
    EffectiveStats AccumulatedStats;
    FloatTy        OptimizeValue = 0;
    int            EchoIndex     = -1;
};

template <long long N>
constexpr long long
Factorial( )
{
    return N * Factorial<N - 1>( );
}
template <>
constexpr long long
Factorial<0>( )
{
    return 1;
}

constexpr auto
GetSetIndexMap( )
{
    std::array<std::array<int, eEchoSetCount>, eEchoSetCount> Result { };

    int UniqueIndex = 0;

    for ( int i = 0; i < eEchoSetCount; ++i )
    {
        for ( int j = 0; j < eEchoSetCount; ++j )
        {
            if ( j >= i )
            {
                Result[ i ][ j ] = UniqueIndex;
                ++UniqueIndex;
            } else
            {
                Result[ i ][ j ] = Result[ j ][ i ];
            }
        }
    }

    return std::pair { Result, UniqueIndex };
}

constexpr auto SetIndexMap = GetSetIndexMap( ).first;

template <int MaxCost = 12, int MaxSlot = 5>
auto
knapsack( const std::vector<EffectiveStats>& Echos, auto&& BaseAttack )
{
    constexpr auto CostSize = MaxCost + 1;
    using CostCombination   = std::array<StatsCache, CostSize>;
    constexpr auto SlotSize = MaxSlot + 1;
    using SlotCombination   = std::array<CostCombination, SlotSize>;
    constexpr auto SetSize  = GetSetIndexMap( ).second;
    using SetCombination    = std::array<SlotCombination, SetSize>;

    SetCombination Caches;

    for ( const auto& [ Index, Echo ] : Echos | std::views::enumerate )
    {
        int EchoCost = Echo.Cost;

        for ( const auto& MemberOfSet : SetIndexMap[ Echo.Set ] )
        {
            auto& SetToOptimize = Caches[ MemberOfSet ];

            for ( int SlotAvailable = MaxSlot; SlotAvailable >= 1; --SlotAvailable )
            {
                auto& SubOptimizeSlot = SetToOptimize[ SlotAvailable - 1 ];
                auto& CurrentSlot     = SetToOptimize[ SlotAvailable ];

                for ( int CoseLeft = MaxCost; CoseLeft >= EchoCost; --CoseLeft )
                {
                    const auto& SubOptimizeBest = SubOptimizeSlot[ CoseLeft - EchoCost ];
                    const auto  NewStats        = SubOptimizeBest.AccumulatedStats + Echo;

                    const auto NewValue = NewStats.ExpectedDamage( BaseAttack );
                    if ( NewValue > CurrentSlot[ CoseLeft ].OptimizeValue )
                    {
                        CurrentSlot[ CoseLeft ] = StatsCache {
                            .AccumulatedStats = NewStats,
                            .OptimizeValue    = NewValue,
                            .EchoIndex        = (int) Index };
                    }
                }
            }
        }
    }

    int MaxSetIndex =
        std::distance( Caches.begin( ),
                       std::ranges::max_element( Caches, []( auto& SetA, auto& SetB ) {
                           return SetA[ MaxSlot ][ MaxCost ].OptimizeValue
                               < SetB[ MaxSlot ][ MaxCost ].OptimizeValue;
                       } ) );

    std::vector<std::decay_t<decltype( Echos )>::const_iterator> Result;

    const auto& MaxSet          = Caches[ MaxSetIndex ];
    int         MaxValCostIndex = MaxCost;
    int         MaxValSlotIndex = MaxSlot;
    while ( true )
    {
        auto EchoIndex = MaxSet[ MaxValSlotIndex ][ MaxValCostIndex ].EchoIndex;
        if ( EchoIndex < 0 ) break;

        Result.emplace_back( Echos.begin( ) + EchoIndex );
        MaxValSlotIndex -= 1;
        MaxValCostIndex -= Echos[ EchoIndex ].Cost;
    }

    return Result;
}

struct CombinationRecord {
    uint64_t CombinationData = 0;
    FloatTy  Value           = 0;
    int      SlotCount       = 5;

    static constexpr auto IndexMaskBitCount = 12;
    static constexpr auto IndexMask         = ( (uint64_t) 1 << IndexMaskBitCount ) - 1;

    constexpr auto& SetAt( uint64_t Data, int Slot ) noexcept
    {
        assert( Data >= 0 && Data <= IndexMask );
        assert( Slot >= 0 && Slot < SlotCount );

        const auto Shift = Slot * IndexMaskBitCount;
        CombinationData &= ~( IndexMask << Shift );
        CombinationData |= Data << Shift;

        return *this;
    }

    [[nodiscard]] constexpr auto GetAt( int Slot ) const noexcept
    {
        assert( Slot >= 0 && Slot < SlotCount );

        const auto Shift = Slot * IndexMaskBitCount;
        return ( CombinationData & ( IndexMask << Shift ) ) >> Shift;
    }

    constexpr bool operator( )( CombinationRecord& obj1, CombinationRecord& obj2 ) const noexcept
    {
        return obj1.Value > obj2.Value;
    }

    std::string ToString( ) const
    {
        std::stringstream ss;
        ss << Value << " : ";
        for ( int i = 0; i < SlotCount; ++i )
        {
            if ( GetAt( i ) == IndexMask ) break;

            ss << GetAt( i ) << " -> ";
        }
        ss << "nul";

        return ss.str( );
    }
};


std::ostream&
operator<<( std::ostream& os, const CombinationRecord& obj ) noexcept
{
    os << obj.ToString( );
    return os;
}

constexpr size_t
BinomialCoefficient( size_t n, size_t k )
{
    // if ( k > n ) return 0;

    int res = 1;

    // Since C(n, k) = C(n, n-k)
    if ( k > n - k )
        k = n - k;

    // Calculate value of
    // [n * (n-1) *---* (n-k+1)] / [k * (k-1) *----* 1]
    for ( size_t i = 0; i < k; ++i )
    {
        res *= ( n - i );
        res /= ( i + 1 );
    }

    return res;
}

constexpr size_t
GetPossibleCombinationCount( const std::vector<EffectiveStats>& Echos ) noexcept
{
    const auto EchoCounts = Echos
        | std::views::chunk_by( []( auto& A, auto& B ) { return A.Cost == B.Cost; } )
        | std::views::transform( []( const auto& Range ) -> int { return std::ranges::distance( Range ); } )
        | std::ranges::to<std::vector>( );

    const int FourCount  = EchoCounts[ 0 ];
    const int ThreeCount = EchoCounts[ 1 ];
    const int OneCount   = EchoCounts[ 2 ];

    /* Make sure all combinations is constructable */
    assert( FourCount >= 3 );
    assert( ThreeCount >= 4 );
    assert( OneCount >= 5 );

    // clang-format off
        return
                                                                                            BinomialCoefficient(FourCount, 3)+// 4 4 4
        BinomialCoefficient(FourCount, 2) * BinomialCoefficient(ThreeCount, 1) * BinomialCoefficient(OneCount, 1)+ // 4 4 3 1
        BinomialCoefficient(FourCount, 0) * BinomialCoefficient(ThreeCount, 4) * BinomialCoefficient(OneCount, 0)+ // 3 3 3 3
        BinomialCoefficient(FourCount, 2) * BinomialCoefficient(ThreeCount, 0) * BinomialCoefficient(OneCount, 3)+ // 4 4 1 1 1
        BinomialCoefficient(FourCount, 1) * BinomialCoefficient(ThreeCount, 0) * BinomialCoefficient(OneCount, 4)+ // 4 1 1 1 1
        BinomialCoefficient(FourCount, 1) * BinomialCoefficient(ThreeCount, 2) * BinomialCoefficient(OneCount, 2)+ // 4 3 3 1 1
        BinomialCoefficient(FourCount, 1) * BinomialCoefficient(ThreeCount, 1) * BinomialCoefficient(OneCount, 3)+ // 4 3 1 1 1
        BinomialCoefficient(FourCount, 0) * BinomialCoefficient(ThreeCount, 1) * BinomialCoefficient(OneCount, 4)+ // 3 1 1 1 1
        BinomialCoefficient(FourCount, 0) * BinomialCoefficient(ThreeCount, 2) * BinomialCoefficient(OneCount, 3)+ // 3 3 1 1 1
        BinomialCoefficient(FourCount, 0) * BinomialCoefficient(ThreeCount, 3) * BinomialCoefficient(OneCount, 2)+ // 3 3 3 1 1
        BinomialCoefficient(FourCount, 0) * BinomialCoefficient(ThreeCount, 0) * BinomialCoefficient(OneCount, 5); // 1 1 1 1 1
    // clang-format on
}

template <class T, class S, class C>
auto&
GetContainer( std::priority_queue<T, S, C>& q )
{
    struct HackedQueue : private std::priority_queue<T, S, C> {
        static S& Container( std::priority_queue<T, S, C>& q )
        {
            return q.*&HackedQueue::c;
        }
    };
    return HackedQueue::Container( q );
}

struct BruteForceSearchRealTimeStat {
    std::size_t PossibleCombinationCount = 0;
    std::size_t CombinationCountSearched = 0;
    std::priority_queue<CombinationRecord,
                        std::vector<CombinationRecord>,
                        CombinationRecord>
        MinHeapMaxCombinations;
} BFSRealTimeStat;

template <int ResultLength = 10, int MaxCost = 12, int MaxSlot = 5>
auto
BruteForce( const std::vector<EffectiveStats>& Echos, auto&& BaseAttack )
{
    const auto BinomialCoefficient = Echos.size( );

    const auto PossibleCombinationCount = GetPossibleCombinationCount( Echos );
    spdlog::info( "All possible combination count: {}", PossibleCombinationCount );
    BFSRealTimeStat.PossibleCombinationCount = PossibleCombinationCount;

    CombinationRecord CurrentCombination;
    CurrentCombination
        .SetAt( CombinationRecord::IndexMask, 0 )
        .SetAt( CombinationRecord::IndexMask, 1 )
        .SetAt( CombinationRecord::IndexMask, 2 )
        .SetAt( CombinationRecord::IndexMask, 3 )
        .SetAt( CombinationRecord::IndexMask, 4 );

    auto& MinHeapMaxCombinations = BFSRealTimeStat.MinHeapMaxCombinations;
    while ( !MinHeapMaxCombinations.empty( ) )
        MinHeapMaxCombinations.pop( );
    for ( int i = 0; i < ResultLength; ++i )
        MinHeapMaxCombinations.push( { .Value = std::numeric_limits<FloatTy>::min( ) } );

    std::array<EffectiveStats, MaxSlot + /* For empty root node */ 1> AccEchoStack;
    std::array<int16_t, MaxSlot + /* For empty root node */ 1>        IndexStack { };
    IndexStack.fill( -1 );

    const auto EchoCostComp = []( auto&& EchoA, auto&& EchoB ) { return EchoA.Cost > EchoB.Cost; };
    if ( !std::ranges::is_sorted( Echos, EchoCostComp ) )
    {
        throw std::runtime_error( "BruteForce: Echos is not sorted by cost" );
    }

    std::array<int, 4 + 1> CostStart { };
    CostStart[ 0 ] = Echos.size( );
    CostStart[ 1 ] = std::distance( Echos.begin( ), std::ranges::lower_bound( Echos, EffectiveStats { .Cost = 1 }, EchoCostComp ) );
    CostStart[ 2 ] = std::distance( Echos.begin( ), std::ranges::lower_bound( Echos, EffectiveStats { .Cost = 2 }, EchoCostComp ) );
    CostStart[ 3 ] = std::distance( Echos.begin( ), std::ranges::lower_bound( Echos, EffectiveStats { .Cost = 3 }, EchoCostComp ) );
    CostStart[ 4 ] = std::distance( Echos.begin( ), std::ranges::lower_bound( Echos, EffectiveStats { .Cost = 4 }, EchoCostComp ) );

    std::array<int, 4 + 1> NextSmaller { };
    NextSmaller[ 0 ] = CostStart[ 0 ];
    NextSmaller[ 1 ] = CostStart[ 0 ];
    NextSmaller[ 2 ] = CostStart[ 1 ];
    NextSmaller[ 3 ] = CostStart[ 2 ];
    NextSmaller[ 4 ] = CostStart[ 3 ];

    int  CurrentSlot = /* Skip first empty root node */ 1;
    auto CostLeft    = MaxCost;

    BFSRealTimeStat.CombinationCountSearched = 0;

    while ( true )
    {
        auto& EchoIndexToEquip = ++IndexStack[ CurrentSlot ];

        // Reach the end
        if ( EchoIndexToEquip >= BinomialCoefficient )
        {
            if ( CurrentSlot == 1 ) [[unlikely]]
            {
                // Tried all possible combinations
                break;
            }

            // Unequip current echo
            CostLeft += Echos[ IndexStack[ CurrentSlot - 1 ] ].Cost;
            CurrentCombination.SetAt( CombinationRecord::IndexMask, CurrentSlot - 2 );

            // Goto previous slot
            CurrentSlot--;
            continue;
        }

        // Make sure cost is not exceeded
        if ( CostLeft < Echos[ EchoIndexToEquip ].Cost )
        {
            // Check if we can skip to next echo equipable
            EchoIndexToEquip = CostStart[ CostLeft ] - 1;
            if ( EchoIndexToEquip != BinomialCoefficient - 1 )
            {
                // Skip to next echo equipable
                continue;
            }

            // Echos is sorted by cost, so we can stop here
            // Goto previous slot
            CurrentSlot--;
            continue;
        }

        // Equip echo
        const auto EchoCost = Echos[ EchoIndexToEquip ].Cost;
        CostLeft -= EchoCost;
        CurrentCombination.SetAt( EchoIndexToEquip, CurrentSlot - 1 );

        AccEchoStack[ CurrentSlot ] = AccEchoStack[ CurrentSlot - 1 ] + Echos[ EchoIndexToEquip ];

        // clang-format off
        if (
                // =======================================
                // Check if current combination is the end
                // =======================================

                /* All slot occupied         */  CurrentSlot == MaxSlot
            ||  /* Cost reached maximum      */  CostLeft == 0
            ||  /* No more echo is equipable */  CostLeft <= 4 && CostStart[ CostLeft ] == BinomialCoefficient
        )
        // clang-format on
        {
            // Calculate combination damage
            CurrentCombination.Value = AccEchoStack[ CurrentSlot ].ExpectedDamage( BaseAttack );
            if ( MinHeapMaxCombinations.top( ).Value < CurrentCombination.Value )
            {
                MinHeapMaxCombinations.pop( );
                MinHeapMaxCombinations.push( CurrentCombination );
                spdlog::info( "Progress: {}% ({}) New record - {}",
                              (FloatTy) BFSRealTimeStat.CombinationCountSearched * 100 / PossibleCombinationCount,
                              BFSRealTimeStat.CombinationCountSearched,
                              CurrentCombination.ToString( ) );
            }

            ++BFSRealTimeStat.CombinationCountSearched;

            // Unequip echo
            CostLeft += EchoCost;
            CurrentCombination.SetAt( CombinationRecord::IndexMask, CurrentSlot - 1 );

        } else
        {
            // Keep echo and goto next slot
            ++CurrentSlot;

            // Make sure don't waste time calculating the same combination
            IndexStack[ CurrentSlot ] = IndexStack[ CurrentSlot - 1 ];

            continue;
        }
    }

    if ( CostLeft != MaxCost )
    {
        throw std::runtime_error( "BruteForce: Login error: CostLeft != MaxCost" );
    }

    spdlog::info( "CombinationCount: {}", BFSRealTimeStat.CombinationCountSearched );

    std::vector<std::vector<std::decay_t<decltype( Echos )>::const_iterator>> Result;

    auto HeapCopy = GetContainer( MinHeapMaxCombinations );
    for ( int HeapCount = HeapCopy.size( ); HeapCount > 0; --HeapCount )
    {
        const auto& Combination = std::ranges::pop_heap( HeapCopy.begin( ),
                                                         HeapCopy.begin( ) + HeapCount,
                                                         CombinationRecord { } )
            - 1;
        Result.emplace_back( );
        for ( int i = 0; i < MaxSlot; ++i )
        {
            if ( Combination->GetAt( i ) == CombinationRecord::IndexMask ) break;

            const auto EchoIndex = Combination->GetAt( i );
            Result.back( ).push_back( Echos.begin( ) + EchoIndex );
        }
    }

    std::ranges::reverse( Result );
    return Result;
}

template <std::size_t Count>
struct BoolArray {
    std::array<uint8_t, Count / 8 + 1> Value { 0 };
    int                                TrueCount = 0;

    constexpr void Reset( )
    {
        TrueCount = 0;
        memset( Value.data( ), 0, sizeof( Value ) );
    }

    constexpr bool GetAt( int Index ) const
    {
        return ( Value[ Index / 8 ] >> ( Index % 8 ) );
    }

    template <bool Data = true>
    constexpr bool SetAt( int Index )
    {
        auto&      Slot      = Value[ Index / 8 ];
        const auto BitAtSlot = 1 << ( Index % 8 );

        if constexpr ( Data )
        {
            if ( ~Slot & BitAtSlot )
            {
                Slot |= BitAtSlot;
                TrueCount += 1;
                return true;
            }
        } else
        {
            if ( Slot & BitAtSlot )
            {
                Slot &= ~BitAtSlot;
                TrueCount -= 1;
                return true;
            }
        }

        return false;
    }
};

struct GARuntimeReport {
    static constexpr auto MaxCombinationCount = 11;

    FloatTy OptimalValue[ MaxCombinationCount ];
    FloatTy MutationProb[ MaxCombinationCount ];

    std::array<std::vector<int>, MaxCombinationCount> ParentPickCount;

    struct DetailReportQueue {
        std::set<uint64_t> QueueSet;
        std::priority_queue<CombinationRecord,
                            std::vector<CombinationRecord>,
                            CombinationRecord>
            Queue;
    };

    std::array<DetailReportQueue, MaxCombinationCount> DetailReports;

} GAReport;

template <int SlotCount = 5, int ResultLength = 10, int PopulationSize = 10000, int ReproduceSize = PopulationSize / 100>
auto
GeneticAlgorithm( int GAReportIndex, const std::vector<EffectiveStats>& Echos, auto&& BaseAttack, std::vector<int>&& FixedCostAtSlot )
{
    static constexpr int MaxEchoCount = 2000;

    if ( !std::ranges::is_sorted( Echos, []( auto&& EchoA, auto&& EchoB ) { return EchoA.Cost > EchoB.Cost; } ) )
    {
        throw std::runtime_error( "GeneticAlgorithm: Echos is not sorted by cost" );
    }

    if ( 2000 < Echos.size( ) )
    {
        throw std::runtime_error( "GeneticAlgorithm: Echos is too much" );
    }

    if ( SlotCount != FixedCostAtSlot.size( ) )
    {
        throw std::range_error( "FixedCostAtSlot is not with size SlotCount" );
    }

    GARuntimeReport::DetailReportQueue& DetailReport = GAReport.DetailReports[ GAReportIndex ];

    auto& MinHeapMaxCombinationsSet = DetailReport.QueueSet;
    MinHeapMaxCombinationsSet.clear( );
    auto& MinHeapMaxCombinations = DetailReport.Queue;
    while ( !MinHeapMaxCombinations.empty( ) )
        MinHeapMaxCombinations.pop( );
    for ( int i = 0; i < ResultLength; ++i )
        MinHeapMaxCombinations.push( { .Value = std::numeric_limits<FloatTy>::min( ), .SlotCount = SlotCount } );

    using RNG = XoshiroCpp::Xoshiro256Plus;
    RNG random { std::random_device { }( ) };

    std::uniform_real_distribution<FloatTy> real_dist( 0, 1 );
    std::exponential_distribution<FloatTy>  expo_dist( 1.0 );

    std::array<int, 5> CountByFixedCost { 0,
                                          (int) std::ranges::count( FixedCostAtSlot, 1 ),
                                          0,
                                          (int) std::ranges::count( FixedCostAtSlot, 3 ),
                                          (int) std::ranges::count( FixedCostAtSlot, 4 ) };

    std::array<std::pair<int, int>, 5> LowerBoundCountByFixedCost;
    for ( int i = 0; i < 5; ++i )
    {
        LowerBoundCountByFixedCost[ i ] =
            std::pair<int, int> {
                std::ranges::lower_bound( FixedCostAtSlot, i, std::ranges::greater { } ) - FixedCostAtSlot.begin( ),
                std::ranges::upper_bound( FixedCostAtSlot, i, std::ranges::greater { } ) - FixedCostAtSlot.begin( ) };
        LowerBoundCountByFixedCost[ i ].second = LowerBoundCountByFixedCost[ i ].second - LowerBoundCountByFixedCost[ i ].first;
    }

    using EquipmentIndex = int;
    std::vector<std::array<EquipmentIndex, SlotCount>> Population( PopulationSize );

    const auto EchoCounts = Echos
        | std::views::chunk_by( []( auto& A, auto& B ) { return A.Cost == B.Cost; } )
        | std::views::transform( []( const auto& Range ) -> int { return std::ranges::distance( Range ); } )
        | std::ranges::to<std::vector>( );
    const int FourCount  = EchoCounts[ 0 ];
    const int ThreeCount = EchoCounts[ 1 ];
    const int OneCount   = EchoCounts[ 2 ];

    std::vector<std::array<int, SlotCount>> IndicesPermutations;
    {
        std::array<int, SlotCount> Permutation;
        std::ranges::iota( Permutation, 0 );
        do
        {
            IndicesPermutations.push_back( Permutation );
        } while ( std::ranges::next_permutation( Permutation ).found );
    }

    std::vector<int> EchoIndices( Echos.size( ) );
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
     * Initialize population with random echos
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

    auto& MutationProbability = GAReport.MutationProb[ GAReportIndex ];
    auto& OptimalValue        = GAReport.OptimalValue[ GAReportIndex ];
    OptimalValue              = std::numeric_limits<FloatTy>::lowest( );

    GAReport.ParentPickCount[ GAReportIndex ].resize( ReproduceSize );

    Stopwatch SW;

    // Pre allocated memories
    std::array<std::pair<FloatTy, int64_t>, PopulationSize> Fitness_Index;
    std::array<std::pair<FloatTy, int64_t>, ReproduceSize>  TopPopulationFitness_Index;
    std::array<BoolArray<MaxEchoCount>, 5>                  IndexSets;
    std::array<std::vector<int>, 5>                         ReproduceIndices;

    while ( true )
    {
        /*
         *
         * Selection
         *
         * */
        memset( TopPopulationFitness_Index.data( ), 0, sizeof( TopPopulationFitness_Index ) );
        for ( int i = 0, top_count = 0; i < PopulationSize; ++i )
        {
            const auto Fitness = std::ranges::fold_left(
                                     Population[ i ],
                                     EffectiveStats { }, [ &Echos ]( auto&& Stat, int EchoIndex ) {
                                         return Stat + Echos[ EchoIndex ];
                                     } )
                                     .ExpectedDamage( BaseAttack );

            if ( top_count < ReproduceSize ) [[unlikely]]
            {
                TopPopulationFitness_Index[ top_count++ ] = std::make_pair( Fitness, i );
                std::ranges::push_heap( TopPopulationFitness_Index.begin( ), TopPopulationFitness_Index.begin( ) + top_count, std::greater<> { } );
            } else if ( TopPopulationFitness_Index[ 0 ].first < Fitness )
            {
                std::ranges::pop_heap( TopPopulationFitness_Index, std::greater<> { } );
                TopPopulationFitness_Index[ ReproduceSize - 1 ] = std::make_pair( Fitness, i );
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

                std::array<int, SlotCount> SortedIndices;
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

        for ( std::array<EquipmentIndex, SlotCount>& Individual : std::ranges::subrange( Population.begin( ) + ReproduceSize, Population.end( ) ) )
        {
            /*
             *
             * Crossover
             *
             * */
            constexpr auto ReproduceSizeBy5 = ReproduceSize / 5;
            const auto     FirstPickIndex   = std::abs( static_cast<int>( expo_dist( random ) * ReproduceSizeBy5 ) % ReproduceSize );
            const auto     SecondPickIndex  = std::abs( static_cast<int>( expo_dist( random ) * ReproduceSizeBy5 ) % ReproduceSize );

            GAReport.ParentPickCount[ GAReportIndex ][ FirstPickIndex ]++;
            GAReport.ParentPickCount[ GAReportIndex ][ SecondPickIndex ]++;

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
            AppendByIndices( IndicesPermutations[ random( ) % Factorial<SlotCount>( ) ], ReproduceIndices[ 4 ], IndividualIt, CountByFixedCost[ 4 ] );
            AppendByIndices( IndicesPermutations[ random( ) % Factorial<SlotCount>( ) ], ReproduceIndices[ 3 ], IndividualIt, CountByFixedCost[ 3 ] );
            AppendByIndices( IndicesPermutations[ random( ) % Factorial<SlotCount>( ) ], ReproduceIndices[ 1 ], IndividualIt, CountByFixedCost[ 1 ] );
            assert( IndividualIt == Individual.end( ) );

            /*
             *
             * Mutation
             *
             * */
            while ( real_dist( random ) < MutationProbability )
            {
                const auto MutationAtCost = FixedCostAtSlot[ random( ) % SlotCount ];

                auto [ MutationStart, MutationCount ] = LowerBoundCountByFixedCost[ MutationAtCost ];

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

        // Write to first ReproduceSize th element in population
        std::ranges::copy(
            TopPopulationFitness_Index
                | std::views::transform( [ &Population ]( auto& FI ) { return Population[ FI.second ]; } ),
            Population.begin( ) );
    }

    spdlog::info( "Final rand: {}", random( ) );

    // For visualizer
    MutationProbability = -1;
    return;
}

int
main( )
{
    std::ios::sync_with_stdio( false );
    std::cin.tie( nullptr );

    const std::vector<FullStats> FullStatsList = json::parse( std::ifstream { "data/example_echos.json" } )[ "echos" ];

    std::vector<EffectiveStats> EffectiveStatsList =
        FullStatsList        // | std::views::filter( []( const FullStats& FullEcho ) { return FullEcho.Level != 0; } )
        // | std::views::filter( []( const FullStats& FullEcho ) { return FullEcho.Set == eMoltenRift || FullEcho.Set == eFreezingFrost; } )
        | std::views::transform( ToEffectiveStats<eFireDamagePercentage, eAutoAttackDamagePercentage> )
        | std::ranges::to<std::vector>( );

    std::ranges::sort( EffectiveStatsList, []( const auto& EchoA, const auto& EchoB ) {
        // return EchoA.Cost > EchoB.Cost;

        if ( EchoA.Cost > EchoB.Cost ) return true;
        if ( EchoA.Cost < EchoB.Cost ) return false;
        return EchoA.ExpectedDamage( 200 ) > EchoB.ExpectedDamage( 200 );
    } );

    std::vector<std::unique_ptr<std::jthread>> Threads;

    // clang-format off
    Threads.emplace_back( std::make_unique<std::jthread>([&](){GeneticAlgorithm<3>( 0,  EffectiveStatsList, 500, { 4, 4, 4}        );} ) );
    Threads.emplace_back( std::make_unique<std::jthread>([&](){GeneticAlgorithm<4>( 1,  EffectiveStatsList, 500, { 4, 4, 3, 1 }    );} ) );
    Threads.emplace_back( std::make_unique<std::jthread>([&](){GeneticAlgorithm<4>( 2,  EffectiveStatsList, 500, { 3, 3, 3, 3 }    );} ) );
    Threads.emplace_back( std::make_unique<std::jthread>([&](){GeneticAlgorithm<5>( 3,  EffectiveStatsList, 500, { 4, 4, 1, 1, 1 } );} ) );
    Threads.emplace_back( std::make_unique<std::jthread>([&](){GeneticAlgorithm<5>( 4,  EffectiveStatsList, 500, { 4, 1, 1, 1, 1 } );} ) );
    Threads.emplace_back( std::make_unique<std::jthread>([&](){GeneticAlgorithm<5>( 5,  EffectiveStatsList, 500, { 4, 3, 3, 1, 1 } );} ) );
    Threads.emplace_back( std::make_unique<std::jthread>([&](){GeneticAlgorithm<5>( 6,  EffectiveStatsList, 500, { 4, 3, 1, 1, 1 } );} ) );
    Threads.emplace_back( std::make_unique<std::jthread>([&](){GeneticAlgorithm<5>( 7,  EffectiveStatsList, 500, { 3, 1, 1, 1, 1 } );} ) );
    Threads.emplace_back( std::make_unique<std::jthread>([&](){GeneticAlgorithm<5>( 8,  EffectiveStatsList, 500, { 3, 3, 1, 1, 1 } );} ) );
    Threads.emplace_back( std::make_unique<std::jthread>([&](){GeneticAlgorithm<5>( 9,  EffectiveStatsList, 500, { 3, 3, 3, 1, 1 } );} ) );
    Threads.emplace_back( std::make_unique<std::jthread>([&](){GeneticAlgorithm<5>( 10, EffectiveStatsList, 500, { 1, 1, 1, 1, 1 } );} ) );
    // clang-format on

    //    std::thread SearchThread( [ &EffectiveStatsList ]( ) {
    //        Stopwatch SW;
    //        auto      Result = BruteForce( EffectiveStatsList, 500 );
    //        for ( const auto& Combination : Result )
    //        {
    //            std::cout << std::accumulate( Combination.begin( ), Combination.end( ), EffectiveStats { }, []( auto Total, auto&& Echo ) {
    //                             return Total + *Echo;
    //                         } ).ExpectedDamage( 500 )
    //                      << " : ";
    //
    //            for ( const auto& i : Combination )
    //            {
    //                std::cout << std::distance( EffectiveStatsList.cbegin( ), i ) << "(" << i->Cost << ") -> ";
    //            }
    //            std::cout << "nul" << std::endl;
    //        }
    //    } );

    sf::RenderWindow window( sf::VideoMode( 640, 650 ), "WuWa Optimize" );
    window.setFramerateLimit( 60 );
    if ( !ImGui::SFML::Init( window ) ) return -1;
    ImPlot::CreateContext( );

    static const char* glabels[] = { "444", "4431", "3333", "44111", "41111", "43311", "43111", "31111", "33111", "33311", "11111" };

    sf::Clock deltaClock;
    while ( window.isOpen( ) )
    {
        sf::Event event { };
        while ( window.pollEvent( event ) )
        {
            ImGui::SFML::ProcessEvent( window, event );

            if ( event.type == sf::Event::Closed )
            {
                window.close( );
            }
        }

        ImGui::SFML::Update( window, deltaClock.restart( ) );

        static bool             use_work_area = true;
        static ImGuiWindowFlags flags         = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

        const ImGuiViewport* viewport = ImGui::GetMainViewport( );
        ImGui::SetNextWindowPos( use_work_area ? viewport->WorkPos : viewport->Pos );
        ImGui::SetNextWindowSize( use_work_area ? viewport->WorkSize : viewport->Size );
        if ( ImGui::Begin( "Display", nullptr, flags ) )
        {
            ImGui::ProgressBar( std::ranges::fold_left( GAReport.MutationProb, 0.f, []( auto A, auto B ) {
                                    return A + ( B <= 0 ? 1 : B );
                                } ) / GARuntimeReport::MaxCombinationCount,
                                ImVec2( -1.0f, 0.0f ) );
            if ( ImPlot::BeginPlot( "Overview" ) )
            {
                // Labels and positions
                static const double  positions[]    = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
                static const FloatTy positionsn05[] = { -0.1, 0.9, 1.9, 2.9, 3.9, 4.9, 5.9, 6.9, 7.9, 8.9, 9.9 };
                static const FloatTy positionsp05[] = { 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1, 10.1 };

                // Setup
                ImPlot::SetupLegend( ImPlotLocation_South, ImPlotLegendFlags_Outside );
                ImPlot::SetupAxes( "Type", "Optimal Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit );

                // Set axis ticks
                ImPlot::SetupAxisTicks( ImAxis_X1, positions, GARuntimeReport::MaxCombinationCount, glabels );
                ImPlot::SetupAxisLimits( ImAxis_X1, -1, 11, ImPlotCond_Always );

                ImPlot::SetupAxis( ImAxis_X2, "Type", ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoDecorations );
                ImPlot::SetupAxisLimits( ImAxis_X2, -1, 11, ImPlotCond_Always );

                ImPlot::SetupAxis( ImAxis_Y2, "Progress", ImPlotAxisFlags_AuxDefault );
                ImPlot::SetupAxisLimits( ImAxis_Y2, 0, 1, ImPlotCond_Always );

                // Plot
                ImPlot::SetAxes( ImAxis_X1, ImAxis_Y1 );
                ImPlot::PlotBars( "Optimal Value", positionsn05, GAReport.OptimalValue, GARuntimeReport::MaxCombinationCount, 0.2 );

                ImPlot::SetAxes( ImAxis_X2, ImAxis_Y2 );
                ImPlot::PlotBars( "Progress", positionsp05, GAReport.MutationProb, GARuntimeReport::MaxCombinationCount, 0.2 );

                ImPlot::EndPlot( );
            }

            if ( ImPlot::BeginPlot( "Details" ) )
            {
                ImPlot::SetupAxes( "Rank", "Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit );

                for ( int i = 0; i < GARuntimeReport::MaxCombinationCount; i++ )
                {
                    auto CopyList = GetContainer( GAReport.DetailReports[ i ].Queue );
                    if ( CopyList.size( ) >= 10 )
                    {

                        auto Values =
                            CopyList
                            | std::views::transform( []( auto& C ) { return C.Value; } )
                            | std::ranges::to<std::vector>( );

                        std::ranges::sort( Values, std::greater { } );
                        ImPlot::PlotLine( glabels[ i ], Values.data( ), Values.size( ) );
                    }
                }

                ImPlot::EndPlot( );
            }
        }
        ImGui::End( );

        window.clear( );
        ImGui::SFML::Render( window );
        window.display( );
    }

    ImPlot::DestroyContext( );
    ImGui::SFML::Shutdown( );

    return 0;
}