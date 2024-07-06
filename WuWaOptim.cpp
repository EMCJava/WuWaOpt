#include <fstream>
#include <iostream>
#include <ranges>
#include <utility>
#include <queue>
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

std::array<ImU32, eEchoSetCount + 1> EchoSetColor {
    IM_COL32( 66, 178, 255, 255 ),
    IM_COL32( 245, 118, 790, 255 ),
    IM_COL32( 182, 108, 255, 255 ),
    IM_COL32( 86, 255, 183, 255 ),
    IM_COL32( 247, 228, 107, 255 ),
    IM_COL32( 204, 141, 181, 255 ),
    IM_COL32( 135, 189, 41, 255 ),
    IM_COL32( 255, 255, 255, 255 ),
    IM_COL32( 202, 44, 37, 255 ),
    IM_COL32( 243, 60, 241, 255 ) };

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

    WuWaGA Opt( FullStatsList );
    Opt.Run<eFireDamagePercentage, eAutoAttackDamagePercentage>( 500 );

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
                        ImGui::Text( std::format( "{:=^43}", Index ).c_str( ) );

                        ImGui::Text( "Set:" );
                        ImGui::SameLine( );
                        ImGui::PushStyleColor( ImGuiCol_Text, EchoSetColor[ SelectedEcho.Set ] );
                        ImGui::Text( std::format( "{:20}", SelectedEcho.GetSetName( ) ).c_str( ) );
                        ImGui::PopStyleColor( );
                        ImGui::SameLine( );
                        ImGui::Text( "%s", SelectedEcho.BriefStat( ).c_str( ) );
                        ImGui::Text( "%s", SelectedEcho.DetailStat( ).c_str( ) );
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