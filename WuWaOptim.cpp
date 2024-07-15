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

#include "Opt/FullStats.hpp"
#include "Opt/OptUtil.hpp"
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

    auto FullStatsList = json::parse( std::ifstream { "data/echos.json" } )[ "echos" ].get<std::vector<FullStats>>( )
        // | std::views::filter( []( const FullStats& FullEcho ) { return FullEcho.Level != 0; } )
        // | std::views::filter( []( const FullStats& FullEcho ) { return FullEcho.Set == eMoltenRift || FullEcho.Set == eFreezingFrost; } )
        | std::views::filter( []( const FullStats& FullEcho ) { return !FullEcho.EchoName.empty( ); } )
        | std::ranges::to<std::vector>( );

    std::ranges::sort( FullStatsList, []( const auto& EchoA, const auto& EchoB ) {
        if ( EchoA.Cost > EchoB.Cost ) return true;
        if ( EchoA.Cost < EchoB.Cost ) return false;
        return false;
    } );

    FloatTy SkillMultiplier     = 0.3877;
    int     CharacterLevel      = 60;
    int     EnemyLevel          = 73;
    FloatTy ElementResistance   = 0.1;
    FloatTy ElementDamageReduce = 0;

    EffectiveStats WeaponStats {
        .flat_attack       = 516,
        .regen             = 0,
        .percentage_attack = 0,
        .buff_multiplier   = 12,
        .crit_rate         = 22.1,
        .crit_damage       = 0,
    };

    EffectiveStats CharacterStats {
        .flat_attack       = 368,
        .regen             = 0,
        .percentage_attack = 12,
        .buff_multiplier   = 2,
        .crit_rate         = 8,
        .crit_damage       = 0,
    };

    const auto GetCommonStat = [ & ]( ) {
        auto CommonStats        = WeaponStats + CharacterStats;
        CommonStats.flat_attack = 0;

        CommonStats.crit_damage /= 100;
        CommonStats.crit_rate /= 100;
        CommonStats.percentage_attack /= 100;
        CommonStats.buff_multiplier /= 100;
        CommonStats.regen /= 100;

        return CommonStats;
    };

    constexpr auto OptimizingElementTy = eLightDamagePercentage;
    constexpr auto OptimizingDamageTy  = eSkillDamagePercentage;

    WuWaGA Opt( FullStatsList );

    sf::RenderWindow window( sf::VideoMode( 600 + 600, 900 ), "WuWa Optimize" );
    window.setFramerateLimit( 60 );
    if ( !ImGui::SFML::Init( window ) ) return -1;
    ImPlot::CreateContext( );

    std::array<std::array<ResultPlotDetails, WuWaGA::ResultLength>, GARuntimeReport::MaxCombinationCount> ResultDisplayBuffer { };

    static const char* glabels[] = { "444", "4431", "3333", "44111", "41111", "43311", "43111", "31111", "33111", "33311", "11111" };

    int            ChosenCombination = -1;
    int            ChosenRank        = 0;
    EffectiveStats SelectedStats;

    const auto DisplayCombination =
        [ & ]( int CombinationIndex, int Rank ) {
            const bool ShowDifferent = ChosenCombination != CombinationIndex || ChosenRank != Rank;

            auto& DisplayCombination = ResultDisplayBuffer[ CombinationIndex ][ Rank ];

            const int      DisplayEchoCount = strlen( glabels[ CombinationIndex ] );
            EffectiveStats DisplayStats     = CalculateCombinationalStat<OptimizingElementTy>(
                std::views::iota( 0, DisplayEchoCount )
                    | std::views::transform( [ & ]( int EchoIndex ) {
                          return Opt.GetEffectiveEchos( )[ DisplayCombination.Indices[ EchoIndex ] ];
                      } ),
                GetCommonStat( ) );

            const auto DisplayRow = [ ShowDifferent ]( const char* Label, FloatTy OldValue, FloatTy Value ) {
                if ( ShowDifferent )
                {
                    ImGui::TableNextRow( );
                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Text( "%s", Label );
                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::Text( "%.3f", Value );
                    if ( Value - OldValue < 0 )
                    {
                        ImGui::SameLine( );
                        ImGui::Text( "(" );
                        ImGui::SameLine( );
                        ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 255, 0, 0, 255 ) );
                        ImGui::Text( "%.3f", Value - OldValue );
                        ImGui::PopStyleColor( );
                        ImGui::SameLine( );
                        ImGui::Text( ")" );
                    } else if ( Value - OldValue > 0 )
                    {
                        ImGui::SameLine( );
                        ImGui::Text( "(" );
                        ImGui::SameLine( );
                        ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 0, 255, 0, 255 ) );
                        ImGui::Text( "+%.3f", Value - OldValue );
                        ImGui::PopStyleColor( );
                        ImGui::SameLine( );
                        ImGui::Text( ")" );
                    }
                } else
                {
                    ImGui::TableNextRow( );
                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Text( "%s", Label );
                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::Text( "%.3f", Value );
                }
            };

            ImGui::SeparatorText( "Effective Stats" );
            if ( ImGui::BeginTable( "EffectiveStats", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg ) )
            {
                const auto BaseAttack = WeaponStats.flat_attack + CharacterStats.flat_attack;

                ImGui::TableSetupColumn( "Stat" );
                ImGui::TableSetupColumn( "Number" );
                ImGui::TableHeadersRow( );

                DisplayRow( "Flat Attack", SelectedStats.flat_attack, DisplayStats.flat_attack );
                DisplayRow( "Regen", SelectedStats.regen * 100 + 100, DisplayStats.regen * 100 + 100 );
                DisplayRow( "Percentage Attack", SelectedStats.percentage_attack * 100, DisplayStats.percentage_attack * 100 );
                DisplayRow( "Buff Multiplier", SelectedStats.buff_multiplier * 100, DisplayStats.buff_multiplier * 100 );
                DisplayRow( "Crit rate", SelectedStats.CritRateStat( ) * 100, DisplayStats.CritRateStat( ) * 100 );
                DisplayRow( "Crit Damage", SelectedStats.CritDamageStat( ) * 100, DisplayStats.CritDamageStat( ) * 100 );
                DisplayRow( "Final Attack", SelectedStats.AttackStat( BaseAttack ), DisplayStats.AttackStat( BaseAttack ) );

                const FloatTy Resistances = ( (FloatTy) 100 + CharacterLevel ) / ( 199 + CharacterLevel + EnemyLevel ) * ( 1 - ElementResistance ) * ( 1 - ElementDamageReduce );
                DisplayRow( "Non Crit Damage", SelectedStats.NormalDamage( BaseAttack ) * SkillMultiplier * Resistances, DisplayStats.NormalDamage( BaseAttack ) * SkillMultiplier * Resistances );
                DisplayRow( "    Crit Damage", SelectedStats.CritDamage( BaseAttack ) * SkillMultiplier * Resistances, DisplayStats.CritDamage( BaseAttack ) * SkillMultiplier * Resistances );
                DisplayRow( "Expected Damage", SelectedStats.ExpectedDamage( BaseAttack ) * SkillMultiplier * Resistances, DisplayStats.ExpectedDamage( BaseAttack ) * SkillMultiplier * Resistances );

                ImGui::EndTable( );
            }

            ImGui::SeparatorText( "Combination" );
            for ( int i = 0; i < DisplayEchoCount; ++i )
            {
                const auto  Index        = DisplayCombination.Indices[ i ];
                const auto& SelectedEcho = FullStatsList[ Index ];
                ImGui::Text( "%s", std::format( "{:=^54}", Index ).c_str( ) );

                ImGui::Text( "Set:" );
                ImGui::SameLine( );
                ImGui::PushStyleColor( ImGuiCol_Text, EchoSetColor[ SelectedEcho.Set ] );
                ImGui::Text( "%s", std::format( "{:31}", SelectedEcho.GetSetName( ) ).c_str( ) );
                ImGui::PopStyleColor( );
                ImGui::SameLine( );
                ImGui::Text( "%s", SelectedEcho.BriefStat( ).c_str( ) );
                ImGui::Text( "%s", SelectedEcho.DetailStat( ).c_str( ) );
            }
        };

    auto&     GAReport = Opt.GetReport( );
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
            {
                ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 5.0f );
                ImGui::BeginChild( "GAStats", ImVec2( 600 - ImGui::GetStyle( ).WindowPadding.x, -1 ), ImGuiChildFlags_Border );

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

                    bool HasData = false;
                    for ( int i = 0; i < GARuntimeReport::MaxCombinationCount; i++ )
                    {
                        std::lock_guard Lock( GAReport.DetailReports[ i ].ReportLock );
                        auto            CopyList = GetConstContainer( GAReport.DetailReports[ i ].Queue );
                        if ( CopyList.size( ) >= WuWaGA::ResultLength )
                        {
                            HasData = true;
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
                    if ( HasData && ImPlot::IsPlotHovered( ) )
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

                        ImGui::BeginTooltip( );
                        DisplayCombination( ClosestCombination, Rank );
                        ImGui::EndTooltip( );

                        // Select the echo combination
                        if ( event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Button::Left )
                        {
                            std::cout << "Left click on " << glabels[ ClosestCombination ] << std::endl;
                            ChosenCombination = ClosestCombination;
                            ChosenRank        = Rank;

                            SelectedStats = CalculateCombinationalStat<OptimizingElementTy>(
                                std::views::iota( 0, (int) strlen( glabels[ ClosestCombination ] ) )
                                    | std::views::transform( [ & ]( int EchoIndex ) {
                                          return Opt.GetEffectiveEchos( )[ SelectedResult.Indices[ EchoIndex ] ];
                                      } ),
                                GetCommonStat( ) );
                        }
                    }

                    ImPlot::EndPlot( );
                }

                ImGui::EndChild( );
                ImGui::PopStyleVar( );
            }

            ImGui::SameLine( );

            {
                ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 5.0f );
                ImGui::BeginChild( "DetailPanel", ImVec2( 600 - ImGui::GetStyle( ).WindowPadding.x * 2, -1 ), ImGuiChildFlags_Border );

                if ( ImGui::Button( "Run", ImVec2( -1, 0 ) ) )
                {
                    const auto BaseAttack = WeaponStats.flat_attack + CharacterStats.flat_attack;
                    Opt.Run<OptimizingElementTy, OptimizingDamageTy>( BaseAttack, GetCommonStat( ) );
                }

                ImGui::SeparatorText( "Weapon" );
                ImGui::DragFloat( "Flat Percentage##1", &WeaponStats.flat_attack, 1, 0, 0, "%.0f" );
                ImGui::DragFloat( "Attack Percentage##1", &WeaponStats.percentage_attack, 0.01, 0, 0, "%.2f" );
                ImGui::DragFloat( "Damage Buff Percentage##1", &WeaponStats.buff_multiplier, 0.01, 0, 0, "%.2f" );
                ImGui::DragFloat( "Crit Rate##1", &WeaponStats.crit_rate, 0.01, 0, 0, "%.2f" );
                ImGui::DragFloat( "Crit Damage##1", &WeaponStats.crit_damage, 0.01, 0, 0, "%.2f" );

                ImGui::SeparatorText( "Character" );
                ImGui::DragFloat( "Flat Percentage##2", &CharacterStats.flat_attack, 1, 0, 0, "%.0f" );
                ImGui::DragFloat( "Attack Percentage##2", &CharacterStats.percentage_attack, 0.01, 0, 0, "%.2f" );
                ImGui::DragFloat( "Damage Buff Percentage##2", &CharacterStats.buff_multiplier, 0.01, 0, 0, "%.2f" );
                ImGui::DragFloat( "Crit Rate##2", &CharacterStats.crit_rate, 0.01, 0, 0, "%.2f" );
                ImGui::DragFloat( "Crit Damage##2", &CharacterStats.crit_damage, 0.01, 0, 0, "%.2f" );

                ImGui::SeparatorText( "Static Configurations" );
                FloatTy SkillMultiplierMul100 = SkillMultiplier * 100;
                ImGui::DragFloat( "Skill Multiplier", &SkillMultiplierMul100, 0.01, 0, 0, "%.2f" );
                SkillMultiplier = SkillMultiplierMul100 / 100;

                ImGui::SliderInt( "Character Level", &CharacterLevel, 1, 90 );
                ImGui::SliderInt( "Enemy Level", &EnemyLevel, 1, 90 );
                ImGui::SliderFloat( "Enemy Element Resistance", &ElementResistance, 0, 1, "%.2f" );
                ImGui::SliderFloat( "Enemy Damage Reduction", &ElementDamageReduce, 0, 1, "%.2f" );

                if ( ChosenCombination >= 0 )
                {
                    DisplayCombination( ChosenCombination, ChosenRank );
                }

                ImGui::EndChild( );
                ImGui::PopStyleVar( );
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