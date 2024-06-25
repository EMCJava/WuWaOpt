#include <fstream>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <utility>
#include <queue>
#include <memory>
#include <set>

#include <imgui.h>   // necessary for ImGui::*, imgui-SFML.h doesn't include imgui.h

#include <imgui-SFML.h>   // for ImGui::SFML::* functions and SFML-specific overloads

#include <implot.h>
#include <implot_internal.h>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <random>

#include "Opt/WuWaGa.hpp"

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

inline std::ostream&
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

template <class T, class S, class C>
auto&
GetConstContainer( const std::priority_queue<T, S, C>& q )
{
    struct HackedQueue : private std::priority_queue<T, S, C> {
        static const S& Container( const std::priority_queue<T, S, C>& q )
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

struct ResultPlotDetails {
    FloatTy            Value;
    std::array<int, 5> Indices;

    bool operator>( ResultPlotDetails Other ) const noexcept
    {
        return Value > Other.Value;
    }
};

ImPlotPoint
ResultPlotDetailsImPlotGetter( int idx, void* user_data )
{
    return { (double) idx, ( ( (ResultPlotDetails*) user_data ) + idx )->Value };
}

int
main( )
{
    std::ios::sync_with_stdio( false );
    std::cin.tie( nullptr );

    auto FullStatsList = json::parse( std::ifstream { "data/example_echos.json" } )[ "echos" ].get<std::vector<FullStats>>( )
        // | std::views::filter( []( const FullStats& FullEcho ) { return FullEcho.Level != 0; } )
        // | std::views::filter( []( const FullStats& FullEcho ) { return FullEcho.Set == eMoltenRift || FullEcho.Set == eFreezingFrost; } )
        | std::ranges::to<std::vector>( );

    std::ranges::sort( FullStatsList, []( const auto& EchoA, const auto& EchoB ) {
        if ( EchoA.Cost > EchoB.Cost ) return true;
        if ( EchoA.Cost < EchoB.Cost ) return false;
    } );

    std::vector<EffectiveStats> EffectiveStatsList =
        FullStatsList
        | std::views::transform( ToEffectiveStats<eFireDamagePercentage, eAutoAttackDamagePercentage> )
        | std::ranges::to<std::vector>( );

    WuWaGA Opt( EffectiveStatsList );
    Opt.Run( );

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

    std::array<std::array<ResultPlotDetails, WuWaGA::ResultLength>, GARuntimeReport::MaxCombinationCount> ResultDisplayBuffer { };

    static const char* glabels[] = { "444", "4431", "3333", "44111", "41111", "43311", "43111", "31111", "33111", "33311", "11111" };

    const auto& GAReport = Opt.GetReport( );
    sf::Clock   deltaClock;
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
                ImPlot::SetupLegend( ImPlotLocation_South, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside );
                ImPlot::SetupAxes( "Rank", "Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit );

                for ( int i = 0; i < GARuntimeReport::MaxCombinationCount; i++ )
                {
                    auto CopyList = GetConstContainer( GAReport.DetailReports[ i ].Queue );
                    if ( CopyList.size( ) >= 10 )
                    {
                        std::ranges::copy( CopyList
                                               | std::views::transform( []( auto& C ) {
                                                     return ResultPlotDetails { C.Value, C.SlotToArray( ) };
                                                 } ),
                                           ResultDisplayBuffer[ i ].begin( ) );

                        std::ranges::sort( ResultDisplayBuffer[ i ], std::greater { } );
                        ImPlot::PlotLineG( glabels[ i ], ResultPlotDetailsImPlotGetter, ResultDisplayBuffer[ i ].data( ), WuWaGA::ResultLength );
                    }
                }

                ImDrawList* draw_list = ImPlot::GetPlotDrawList( );
                if ( ImPlot::IsPlotHovered( ) )
                {
                    ImPlotPoint mouse = ImPlot::GetPlotMousePos( );

                    int Rank  = std::round( mouse.x );
                    int Value = mouse.y;

                    FloatTy MinDiff            = std::numeric_limits<FloatTy>::max( );
                    int     ClosestCombination = 0;
                    for ( int i = 0; i < GARuntimeReport::MaxCombinationCount; i++ )
                    {
                        const auto Diff = std::abs( Value - ResultDisplayBuffer[ i ][ Rank ].Value );
                        if ( Diff < MinDiff && ImPlot::GetCurrentContext( )->CurrentItems->GetLegendItem( i )->Show )
                        {
                            ClosestCombination = i;
                            MinDiff            = Diff;
                        }
                    }

                    auto& SelectedResult = ResultDisplayBuffer[ ClosestCombination ][ Rank ];

                    ImPlot::PushPlotClipRect( );
                    draw_list->AddCircleFilled( ImPlot::PlotToPixels( (float) Rank, SelectedResult.Value ), 5, IM_COL32( 255, 0, 0, 255 ) );
                    ImPlot::PopPlotClipRect( );

                    const int SelectedEchoCount = strlen( glabels[ ClosestCombination ] );

                    ImGui::BeginTooltip( );
                    ImGui::Text( "Echo combination %s", glabels[ ClosestCombination ] );
                    ImGui::Text( "Damage %f", SelectedResult.Value );
                    for ( int i = 0; i < SelectedEchoCount; ++i )
                    {
                        const auto  Index        = SelectedResult.Indices[ i ];
                        const auto& SelectedEcho = FullStatsList[ Index ];
                        ImGui::Text( "[%-4d] Cost: %d Level: %-2d", Index, SelectedEcho.Cost, SelectedEcho.Level );
                    }
                    ImGui::EndTooltip( );
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