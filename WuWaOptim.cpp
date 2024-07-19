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

#include "Opt/OptimizerParmSwitcher.hpp"
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
    int                CombinationID;
    int                CombinationRank;
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

ImPlotPoint
ResultSegmentPlotDetailsImPlotGetter( int idx, void* user_data )
{
    return { std::round( (double) idx / 2 ), ( ( (ResultPlotDetails*) user_data ) + idx )->Value };
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

struct OptimizerConfig {

    OptimizerConfig( )
    {
        ReadConfig( );
    }

    EffectiveStats WeaponStats { };
    EffectiveStats CharacterStats { };

    MultiplierConfig OptimizeMultiplierConfig { };
    int              SelectedElement { };

    int     CharacterLevel { };
    int     EnemyLevel { };
    FloatTy ElementResistance { };
    FloatTy ElementDamageReduce { };

    [[nodiscard]] auto GetResistances( ) const noexcept
    {
        return ( (FloatTy) 100 + CharacterLevel ) / ( 199 + CharacterLevel + EnemyLevel ) * ( 1 - ElementResistance ) * ( 1 - ElementDamageReduce );
    };

    void ReadConfig( )
    {
        static_assert( std::is_trivially_copy_assignable_v<OptimizerConfig> );
        std::ifstream OptimizerConfigFile( "oc.data", std::ios::binary );
        if ( OptimizerConfigFile )
        {
            OptimizerConfigFile.read( (char*) this, sizeof( OptimizerConfig ) );
        }
    }

    void SaveConfig( ) const
    {
        static_assert( std::is_trivially_copy_assignable_v<OptimizerConfig> );
        std::ofstream OptimizerConfigFile( "oc.data", std::ios::binary );
        if ( OptimizerConfigFile )
        {
            OptimizerConfigFile.write( (char*) this, sizeof( OptimizerConfig ) );
        }
    }
};

int
main( int argc, char** argv )
{
    spdlog::set_level( spdlog::level::trace );

    std::string EchoFilePath = "echoes.json";
    if ( argc > 1 )
    {
        EchoFilePath = argv[ 1 ];
    }

    std::ifstream EchoFile { EchoFilePath };
    if ( !EchoFile )
    {
        spdlog::error( "Failed to open echoes file." );
        system( "pause" );
        return 1;
    }

    auto FullStatsList = json::parse( EchoFile )[ "echoes" ].get<std::vector<FullStats>>( )
        // | std::views::filter( []( const FullStats& FullEcho ) { return FullEcho.Level != 0; } )
        // | std::views::filter( []( const FullStats& FullEcho ) { return FullEcho.Set == eMoltenRift || FullEcho.Set == eFreezingFrost; } )
        | std::views::filter( []( const FullStats& FullEcho ) { return !FullEcho.EchoName.empty( ); } )
        | std::ranges::to<std::vector>( );

    if ( FullStatsList.empty( ) )
    {
        spdlog::critical( "No valid echoes found in the provided file." );
        return 1;
    }

    std::ranges::sort( FullStatsList, []( const auto& EchoA, const auto& EchoB ) {
        if ( EchoA.Cost > EchoB.Cost ) return true;
        if ( EchoA.Cost < EchoB.Cost ) return false;
        return false;
    } );

    /*
     *
     * Optimizer Configurations
     *
     * */
    OptimizerConfig OConfig;

    const auto GetCommonStat = [ & ]( ) {
        auto CommonStats        = OConfig.WeaponStats + OConfig.CharacterStats;
        CommonStats.flat_attack = 0;

        CommonStats.crit_damage /= 100;
        CommonStats.crit_rate /= 100;
        CommonStats.percentage_attack /= 100;
        CommonStats.buff_multiplier /= 100;
        CommonStats.regen /= 100;
        CommonStats.auto_attack_buff /= 100;
        CommonStats.heavy_attack_buff /= 100;
        CommonStats.skill_buff /= 100;
        CommonStats.ult_buff /= 100;

        return CommonStats;
    };

    const char* ElementLabel[] = {
        "Fire Damage",
        "Air Damage",
        "Ice Damage",
        "Electric Damage",
        "Dark Damage",
        "Light Damage",
    };

    WuWaGA Opt( FullStatsList );

    constexpr auto   ChartSplitWidth = 700;
    constexpr auto   StatSplitWidth  = 800;
    sf::RenderWindow window( sf::VideoMode( ChartSplitWidth + StatSplitWidth, 1000 ), "WuWa Optimize" );
    window.setFramerateLimit( 60 );
    if ( !ImGui::SFML::Init( window ) ) return -1;
    ImPlot::CreateContext( );

    {
        ImGui::StyleColorsClassic( );
        auto& Style            = ImGui::GetStyle( );
        Style.WindowBorderSize = 0.0f;
        Style.FramePadding.y   = 5.0f;
        Style.FrameBorderSize  = 1.0f;
        Style.FrameRounding    = 12.0f;
    }

    std::array<std::array<ResultPlotDetails, WuWaGA::ResultLength>, GARuntimeReport::MaxCombinationCount> ResultDisplayBuffer { };

    std::array<std::array<ResultPlotDetails, WuWaGA::ResultLength * 2>, GARuntimeReport::MaxCombinationCount> GroupedDisplayBuffer { };
    std::array<int, GARuntimeReport::MaxCombinationCount>                                                     CombinationLegendIndex { };

    static const char* glabels[] = { "444", "4431", "3333", "44111", "41111", "43311", "43111", "31111", "33111", "33311", "11111" };

    int            ChosenCombination = -1;
    int            ChosenRank        = 0;
    EffectiveStats SelectedStats;

    const auto DisplayCombination =
        [ & ]( int CombinationIndex, int Rank ) {
            const bool ShowDifferent = ChosenCombination != CombinationIndex || ChosenRank != Rank;

            auto& DisplayCombination = ResultDisplayBuffer[ CombinationIndex ][ Rank ];

            const int      DisplayEchoCount = strlen( glabels[ CombinationIndex ] );
            EffectiveStats DisplayStats     = OptimizerParmSwitcher::SwitchCalculateCombinationalStat(
                OConfig.SelectedElement,
                std::views::iota( 0, DisplayEchoCount )
                    | std::views::transform( [ & ]( int EchoIndex ) {
                          return Opt.GetEffectiveEchos( )[ DisplayCombination.Indices[ EchoIndex ] ];
                      } )
                    | std::ranges::to<std::vector>( ),
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
                const auto BaseAttack = OConfig.WeaponStats.flat_attack + OConfig.CharacterStats.flat_attack;

                ImGui::TableSetupColumn( "Stat" );
                ImGui::TableSetupColumn( "Number" );
                ImGui::TableHeadersRow( );

                DisplayRow( "Flat Attack", SelectedStats.flat_attack, DisplayStats.flat_attack );
                DisplayRow( "Regen", SelectedStats.regen * 100 + 100, DisplayStats.regen * 100 + 100 );
                DisplayRow( "Percentage Attack", SelectedStats.percentage_attack * 100, DisplayStats.percentage_attack * 100 );

                DisplayRow( "Element Buff %", SelectedStats.buff_multiplier * 100, DisplayStats.buff_multiplier * 100 );
                DisplayRow( "Auto Attack %", SelectedStats.auto_attack_buff * 100, DisplayStats.auto_attack_buff * 100 );
                DisplayRow( "Heavy Attack %", SelectedStats.heavy_attack_buff * 100, DisplayStats.heavy_attack_buff * 100 );
                DisplayRow( "Skill Damage %", SelectedStats.skill_buff * 100, DisplayStats.skill_buff * 100 );
                DisplayRow( "Ult Damage %", SelectedStats.ult_buff * 100, DisplayStats.ult_buff * 100 );

                DisplayRow( "Crit rate", SelectedStats.CritRateStat( ) * 100, DisplayStats.CritRateStat( ) * 100 );
                DisplayRow( "Crit Damage", SelectedStats.CritDamageStat( ) * 100, DisplayStats.CritDamageStat( ) * 100 );
                DisplayRow( "Final Attack", SelectedStats.AttackStat( BaseAttack ), DisplayStats.AttackStat( BaseAttack ) );

                const FloatTy Resistances = OConfig.GetResistances( );
                DisplayRow( "Non Crit Damage", SelectedStats.NormalDamage( BaseAttack, &OConfig.OptimizeMultiplierConfig ) * Resistances, DisplayStats.NormalDamage( BaseAttack, &OConfig.OptimizeMultiplierConfig ) * Resistances );
                DisplayRow( "    Crit Damage", SelectedStats.CritDamage( BaseAttack, &OConfig.OptimizeMultiplierConfig ) * Resistances, DisplayStats.CritDamage( BaseAttack, &OConfig.OptimizeMultiplierConfig ) * Resistances );
                DisplayRow( "Expected Damage", SelectedStats.ExpectedDamage( BaseAttack, &OConfig.OptimizeMultiplierConfig ) * Resistances, DisplayStats.ExpectedDamage( BaseAttack, &OConfig.OptimizeMultiplierConfig ) * Resistances );

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

    const std::string              TopCombinationByTypeTitle = std::format( "Top {} Combinations By Type", WuWaGA::ResultLength );
    const std::string              TopCombinationTitle       = std::format( "Top {} Combinations", WuWaGA::ResultLength );
    std::vector<ResultPlotDetails> TopCombination;

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
                ImGui::BeginChild( "GAStats", ImVec2( ChartSplitWidth - ImGui::GetStyle( ).WindowPadding.x, -1 ), ImGuiChildFlags_Border );

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
                    ImPlot::PlotBars( "Optimal Value", positionsn05, GAReport.OptimalValue.data( ), GARuntimeReport::MaxCombinationCount, 0.2 );

                    ImPlot::SetAxes( ImAxis_X2, ImAxis_Y2 );
                    ImPlot::PlotBars( "Progress", positionsp05, GAReport.MutationProb.data( ), GARuntimeReport::MaxCombinationCount, 0.2 );

                    ImPlot::EndPlot( );
                }

                TopCombination.clear( );
                if ( ImPlot::BeginPlot( TopCombinationByTypeTitle.c_str( ) ) )
                {
                    ImPlot::SetupLegend( ImPlotLocation_South, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside );
                    ImPlot::SetupAxes( "Rank", "Optimal Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit );

                    bool       HasData              = false;
                    const auto StaticStatMultiplier = OConfig.GetResistances( );
                    for ( int i = 0; i < GARuntimeReport::MaxCombinationCount; i++ )
                    {
                        std::lock_guard Lock( GAReport.DetailReports[ i ].ReportLock );
                        auto            CopyList = GetConstContainer( GAReport.DetailReports[ i ].Queue );
                        if ( CopyList.size( ) >= WuWaGA::ResultLength )
                        {
                            HasData = true;
                            std::ranges::copy( CopyList
                                                   | std::views::transform( [ i, StaticStatMultiplier ]( const auto& Record ) {
                                                         return ResultPlotDetails { .Value           = Record.Value * StaticStatMultiplier,
                                                                                    .CombinationID   = i,
                                                                                    .CombinationRank = 0,
                                                                                    .Indices         = Record.SlotToArray( ) };
                                                     } ),
                                               ResultDisplayBuffer[ i ].begin( ) );

                            std::ranges::sort( ResultDisplayBuffer[ i ], std::greater { } );

                            for ( auto [ Index, Combination ] : ResultDisplayBuffer[ i ] | std::views::enumerate )
                            {
                                Combination.CombinationRank = (int) Index;
                            }

                            ImPlot::PlotLineG( glabels[ i ], ResultPlotDetailsImPlotGetter, ResultDisplayBuffer[ i ].data( ), WuWaGA::ResultLength );

                            TopCombination.insert( TopCombination.end( ), ResultDisplayBuffer[ i ].begin( ), ResultDisplayBuffer[ i ].end( ) );

                            for ( int i = WuWaGA::ResultLength - 1; i >= 0; --i )
                                std::push_heap( TopCombination.begin( ), TopCombination.end( ) - i, std::greater { } );
                        }
                    }

                    std::sort_heap( TopCombination.begin( ), TopCombination.end( ), std::greater { } );

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
                        if ( sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
                        {
                            ChosenCombination = ClosestCombination;
                            ChosenRank        = Rank;

                            SelectedStats = OptimizerParmSwitcher::SwitchCalculateCombinationalStat(
                                OConfig.SelectedElement,
                                std::views::iota( 0, (int) strlen( glabels[ ClosestCombination ] ) )
                                    | std::views::transform( [ & ]( int EchoIndex ) {
                                          return Opt.GetEffectiveEchos( )[ SelectedResult.Indices[ EchoIndex ] ];
                                      } )
                                    | std::ranges::to<std::vector>( ),
                                GetCommonStat( ) );
                        }
                    }

                    ImPlot::EndPlot( );
                }

                if ( ImPlot::BeginPlot( TopCombinationTitle.c_str( ) ) )
                {
                    ImPlot::SetupLegend( ImPlotLocation_South, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside );
                    ImPlot::SetupAxes( "Rank", "Optimal Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit );

                    if ( !TopCombination.empty( ) )
                    {
                        std::ranges::fill( CombinationLegendIndex, -1 );
                        for ( int i = 0, CombinationIndex = 0; i < GARuntimeReport::MaxCombinationCount; i++ )
                        {
                            bool  HasData       = false;
                            auto& CurrentBuffer = GroupedDisplayBuffer[ i ];
                            for ( int j = 0; j < WuWaGA::ResultLength; ++j )
                            {
                                if ( TopCombination[ j ].CombinationID == i )
                                {
                                    HasData                    = true;
                                    CurrentBuffer[ j * 2 ]     = TopCombination[ j ];
                                    CurrentBuffer[ j * 2 + 1 ] = TopCombination[ j + 1 ];
                                } else
                                {
                                    CurrentBuffer[ j * 2 ] = CurrentBuffer[ j * 2 + 1 ] = ResultPlotDetails { NAN };
                                }
                            }

                            CombinationLegendIndex[ i ] = CombinationIndex;
                            if ( HasData ) CombinationIndex++;
                            ImPlot::PlotLineG( glabels[ i ], ResultSegmentPlotDetailsImPlotGetter, CurrentBuffer.data( ), WuWaGA::ResultLength * 2, ImPlotLineFlags_Segments | ( HasData ? 0 : ImPlotItemFlags_NoLegend ) );
                        }

                        ImDrawList* draw_list = ImPlot::GetPlotDrawList( );
                        if ( ImPlot::IsPlotHovered( ) )
                        {
                            ImPlotPoint mouse = ImPlot::GetPlotMousePos( );
                            int         Rank  = std::round( mouse.x );

                            auto& SelectedResult     = TopCombination[ Rank ];
                            int   ClosestCombination = SelectedResult.CombinationID;
                            int   ClosestRank        = SelectedResult.CombinationRank;

                            if ( ImPlot::GetCurrentContext( )->CurrentItems->GetLegendItem( CombinationLegendIndex[ ClosestCombination ] )->Show )
                            {
                                ImPlot::PushPlotClipRect( );
                                draw_list->AddCircleFilled( ImPlot::PlotToPixels( (float) Rank, SelectedResult.Value ), 5, IM_COL32( 255, 0, 0, 255 ) );
                                ImPlot::PopPlotClipRect( );

                                ImGui::BeginTooltip( );
                                DisplayCombination( ClosestCombination, ClosestRank );
                                ImGui::EndTooltip( );

                                // Select the echo combination
                                if ( sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
                                {
                                    ChosenCombination = ClosestCombination;
                                    ChosenRank        = ClosestRank;

                                    SelectedStats = OptimizerParmSwitcher::SwitchCalculateCombinationalStat(
                                        OConfig.SelectedElement,
                                        std::views::iota( 0, (int) strlen( glabels[ ClosestCombination ] ) )
                                            | std::views::transform( [ & ]( int EchoIndex ) {
                                                  return Opt.GetEffectiveEchos( )[ SelectedResult.Indices[ EchoIndex ] ];
                                              } )
                                            | std::ranges::to<std::vector>( ),
                                        GetCommonStat( ) );
                                }
                            }
                        }
                    }

                    ImPlot::EndPlot( );
                }

                ImGui::EndChild( );
                ImGui::PopStyleVar( );
            }

            ImGui::SameLine( );

            {
#define SAVE_CONFIG( x ) \
    if ( x ) OConfig.SaveConfig( );

                ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 5.0f );
                ImGui::BeginChild( "DetailPanel", ImVec2( StatSplitWidth - ImGui::GetStyle( ).WindowPadding.x * 2, -1 ), ImGuiChildFlags_Border );

                ImGui::BeginChild( "DetailPanel##Character", ImVec2( StatSplitWidth / 2 - ImGui::GetStyle( ).WindowPadding.x * 4, 0 ), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize );
                ImGui::SeparatorText( "Character" );
                SAVE_CONFIG( ImGui::DragFloat( "Flat Attack##Character", &OConfig.CharacterStats.flat_attack, 1, 0, 0, "%.0f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Attack %##Character", &OConfig.CharacterStats.percentage_attack, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Element Buff %##Character", &OConfig.CharacterStats.buff_multiplier, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Auto Attack %##Character", &OConfig.CharacterStats.auto_attack_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Heavy Attack %##Character", &OConfig.CharacterStats.heavy_attack_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Skill Damage %##Character", &OConfig.CharacterStats.skill_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Ult Damage %##Character", &OConfig.CharacterStats.ult_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Crit Rate##Character", &OConfig.CharacterStats.crit_rate, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Crit Damage##Character", &OConfig.CharacterStats.crit_damage, 0.01, 0, 0, "%.2f" ) )
                ImGui::EndChild( );
                ImGui::SameLine( );
                ImGui::BeginChild( "DetailPanel##Weapon", ImVec2( StatSplitWidth / 2 - ImGui::GetStyle( ).WindowPadding.x * 4, 0 ), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize );
                ImGui::SeparatorText( "Weapon" );
                SAVE_CONFIG( ImGui::DragFloat( "Flat Attack##Weapon", &OConfig.WeaponStats.flat_attack, 1, 0, 0, "%.0f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Attack %##Weapon", &OConfig.WeaponStats.percentage_attack, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Element Buff %##Weapon", &OConfig.WeaponStats.buff_multiplier, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Auto Attack %##Weapon", &OConfig.WeaponStats.auto_attack_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Heavy Attack %##Weapon", &OConfig.WeaponStats.heavy_attack_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Skill Damage %##Weapon", &OConfig.WeaponStats.skill_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Ult Damage %##Weapon", &OConfig.WeaponStats.ult_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Crit Rate##Weapon", &OConfig.WeaponStats.crit_rate, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( "Crit Damage##Weapon", &OConfig.WeaponStats.crit_damage, 0.01, 0, 0, "%.2f" ) )
                ImGui::EndChild( );

                ImGui::NewLine( );
                ImGui::Separator( );
                ImGui::NewLine( );

                SAVE_CONFIG( ImGui::DragFloat( "Auto Attack Kit Total %", &OConfig.OptimizeMultiplierConfig.auto_attack_multiplier ) )
                SAVE_CONFIG( ImGui::DragFloat( "Heavy Attack Kit Total %", &OConfig.OptimizeMultiplierConfig.heavy_attack_multiplier ) )
                SAVE_CONFIG( ImGui::DragFloat( "Skill Kit Total %", &OConfig.OptimizeMultiplierConfig.skill_multiplier ) )
                SAVE_CONFIG( ImGui::DragFloat( "Ult Kit Total %", &OConfig.OptimizeMultiplierConfig.ult_multiplier ) )

                ImGui::NewLine( );
                ImGui::Separator( );
                ImGui::NewLine( );

                ImGui::BeginChild( "DetailPanel##Element", ImVec2( StatSplitWidth / 2 - ImGui::GetStyle( ).WindowPadding.x * 4, 0 ), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize );
                SAVE_CONFIG( ImGui::Combo( "Element Type", &OConfig.SelectedElement, ElementLabel, IM_ARRAYSIZE( ElementLabel ) ) )
                ImGui::EndChild( );

                ImGui::NewLine( );
                ImGui::Separator( );
                ImGui::NewLine( );

                const auto  OptRunning = Opt.IsRunning( );
                const float ButtonH    = OptRunning ? 0 : 0.384;
                ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) ImColor::HSV( ButtonH, 0.6f, 0.6f ) );
                ImGui::PushStyleColor( ImGuiCol_ButtonHovered, (ImVec4) ImColor::HSV( ButtonH, 0.7f, 0.7f ) );
                ImGui::PushStyleColor( ImGuiCol_ButtonActive, (ImVec4) ImColor::HSV( ButtonH, 0.8f, 0.8f ) );
                if ( ImGui::Button( OptRunning ? "Re-Run" : "Run", ImVec2( -1, 0 ) ) )
                {
                    const auto BaseAttack = OConfig.WeaponStats.flat_attack + OConfig.CharacterStats.flat_attack;
                    OptimizerParmSwitcher::SwitchRun( Opt, OConfig.SelectedElement, BaseAttack, GetCommonStat( ), &OConfig.OptimizeMultiplierConfig );
                }
                ImGui::PopStyleColor( 3 );

                ImGui::SeparatorText( "Static Configurations" );
                SAVE_CONFIG( ImGui::SliderInt( "Character Level", &OConfig.CharacterLevel, 1, 90 ) )
                SAVE_CONFIG( ImGui::SliderInt( "Enemy Level", &OConfig.EnemyLevel, 1, 90 ) )
                SAVE_CONFIG( ImGui::SliderFloat( "Enemy Element Resistance", &OConfig.ElementResistance, 0, 1, "%.2f" ) )
                SAVE_CONFIG( ImGui::SliderFloat( "Enemy Damage Reduction", &OConfig.ElementDamageReduce, 0, 1, "%.2f" ) )

                if ( ChosenCombination >= 0 )
                {
                    DisplayCombination( ChosenCombination, ChosenRank );
                }

                ImGui::EndChild( );
                ImGui::PopStyleVar( );

#undef SAVE_CONFIG
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