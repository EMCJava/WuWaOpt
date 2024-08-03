#include <fstream>
#include <iostream>
#include <ranges>
#include <utility>
#include <random>
#include <queue>
#include <set>

#include <httplib.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>   // necessary for ImGui::*, imgui-SFML.h doesn't include imgui.h

#include <imgui-SFML.h>   // for ImGui::SFML::* functions and SFML-specific overloads

#include <implot.h>
#include <implot_internal.h>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include "Opt/UI/Backpack.hpp"
#include "Opt/UI/OptimizerUIConfig.hpp"
#include "Opt/UI/PlotCombinationMeta.hpp"
#include "Opt/Tweak/CombinationMetaCache.hpp"
#include "Opt/Tweak/CombinationTweaker.hpp"
#include "Opt/Config/EchoConstraint.hpp"
#include "Opt/Config/OptimizerConfig.hpp"
#include "Opt/SubStatRolling/SubStatRollConfig.hpp"
#include "Opt/OptimizerParmSwitcher.hpp"
#include "Opt/FullStats.hpp"
#include "Opt/OptUtil.hpp"
#include "Opt/WuWaGa.hpp"

#include "Loca/StringArrayObserver.hpp"
#include "Loca/Loca.hpp"

#define WUWAOPT_VERSION "v1.1.7"

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


std::array<ImU32, eEchoSetCount + 1> EchoSetColor {
    IM_COL32( 66, 178, 255, 255 ),
    IM_COL32( 245, 118, 79, 255 ),
    IM_COL32( 182, 108, 255, 255 ),
    IM_COL32( 86, 255, 183, 255 ),
    IM_COL32( 247, 228, 107, 255 ),
    IM_COL32( 204, 141, 181, 255 ),
    IM_COL32( 135, 189, 41, 255 ),
    IM_COL32( 255, 255, 255, 255 ),
    IM_COL32( 202, 44, 37, 255 ),
    IM_COL32( 243, 60, 241, 255 ) };

bool
CompareVersions( const std::string& version1, const std::string& version2 )
{
    std::regex  pattern( R"(v(\d+)\.(\d+)\.(\d+))" );
    std::smatch match1, match2;

    if ( std::regex_match( version1, match1, pattern )
         && std::regex_match( version2, match2, pattern ) )
    {
        for ( int i = 1; i <= 3; ++i )
        {
            int v1 = std::stoi( match1[ i ] );
            int v2 = std::stoi( match2[ i ] );
            if ( v1 < v2 ) return true;
            if ( v1 > v2 ) return false;
        }
    }

    return false;
}

void
CheckForUpdate( auto& LP, auto&& UpdateCallback )
{
    try
    {
        httplib::Client  cli( "https://api.github.com" );
        httplib::Headers headers = {
            { "Accept", "application/vnd.github.v3+json" }
        };
        if ( auto res = cli.Get( "/repos/EMCJava/WuWaOpt/releases/latest", headers ) )
        {
            if ( res->status == 200 )
            {
                const auto& Response      = json::parse( res->body );
                const auto  VersionString = Response[ "tag_name" ].get<std::string>( );
                if ( CompareVersions( WUWAOPT_VERSION, VersionString ) )
                {
                    UpdateCallback( VersionString );
                }
            }
        } else
        {
            throw std::runtime_error( LP[ "FailFetchReleaseVer" ] + to_string( res.error( ) ) );
        }
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "{}", e.what( ) );
    }
}

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
    std::ifstream EchoNameFile { "data/EchoSetNameSet.json" };
    if ( !EchoFile )
    {
        spdlog::error( "Failed to open echoes file." );
        system( "pause" );
        return 1;
    }
    if ( !EchoNameFile )
    {
        spdlog::error( "Failed to open echo name file." );
        system( "pause" );
        return 1;
    }

    std::map<std::string, std::vector<std::string>> EchoNameBySet;
    json::parse( EchoNameFile )[ "sets" ].get_to( EchoNameBySet );
    std::ranges::for_each( EchoNameBySet, []( const auto& NameSet ) {
        assert( NameSet.second.size( ) < std::numeric_limits<SetNameOccupation>::digits );
    } );

    auto FullStatsList = json::parse( EchoFile )[ "echoes" ].get<std::vector<FullStats>>( )
        | std::views::filter( [ & ]( FullStats& FullEcho ) {
                             const auto NameListIt = EchoNameBySet.find( FullEcho.GetSetName( ) );
                             if ( NameListIt == EchoNameBySet.end( ) ) return false;

                             const auto NameIt = std::ranges::find( NameListIt->second, FullEcho.EchoName );
                             if ( NameIt == NameListIt->second.end( ) ) return false;

                             FullEcho.NameID = std::distance( NameListIt->second.begin( ), NameIt );
                             return true;
                         } )
        | std::ranges::to<std::vector>( );

    if ( FullStatsList.empty( ) )
    {
        spdlog::critical( "No valid echoes found in the provided file." );
        return 1;
    }

    std::ranges::sort( FullStatsList, []( const auto& EchoA, const auto& EchoB ) {
        if ( EchoA.Cost > EchoB.Cost ) return true;
        if ( EchoA.Cost < EchoB.Cost ) return false;
        if ( EchoA.Level > EchoB.Level ) return true;
        if ( EchoA.Level < EchoB.Level ) return false;
        return false;
    } );

    /*
     *
     * Optimizer Configurations
     *
     * */
    OptimizerConfig OConfig;
    OConfig.ReadConfig( );

    MultiplierConfig OptimizeMultiplierDisplay {
        .auto_attack_multiplier  = OConfig.OptimizeMultiplierConfig.auto_attack_multiplier * 100,
        .heavy_attack_multiplier = OConfig.OptimizeMultiplierConfig.heavy_attack_multiplier * 100,
        .skill_multiplier        = OConfig.OptimizeMultiplierConfig.skill_multiplier * 100,
        .ult_multiplier          = OConfig.OptimizeMultiplierConfig.ult_multiplier * 100 };

    Loca LanguageProvider( OConfig.LastUsedLanguage );
    spdlog::info( "Using language: {}", LanguageProvider[ "Name" ] );

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

    StringArrayObserver ElementLabel {
        LanguageProvider,
        { "FireDamage",
          "AirDamage",
          "IceDamage",
          "ElectricDamage",
          "DarkDamage",
          "LightDamage" }
    };

    WuWaGA                        Opt;
    std::list<CombinationTweaker> CombinationTweaks;

    constexpr auto   ChartSplitWidth = 700;
    constexpr auto   StatSplitWidth  = 800;
    sf::RenderWindow window( sf::VideoMode( ChartSplitWidth + StatSplitWidth, 1000 ), LanguageProvider.GetDecodedString( "WinTitle" ) );
    window.setFramerateLimit( 60 );
    if ( !ImGui::SFML::Init( window, false ) ) return -1;
    ImPlot::CreateContext( );

    if ( !OConfig.AskedCheckForNewVersion )
    {
        OConfig.ShouldCheckForNewVersion =
            MessageBox(
                nullptr,
                LanguageProvider.GetDecodedString( "OptCheckUpdateQuestion" ).data( ),
                LanguageProvider.GetDecodedString( "OptBeh" ).data( ),
                MB_ICONQUESTION | MB_YESNO | MB_TOPMOST )
            == IDYES;
        OConfig.AskedCheckForNewVersion = true;
        OConfig.SaveConfig( );
    }

    if ( OConfig.ShouldCheckForNewVersion )
    {
        std::thread {
            [ & ]( ) {
                CheckForUpdate(
                    LanguageProvider,
                    [ & ]( const std::string& NewVersion ) {
                        window.setTitle( LanguageProvider.GetDecodedString( "WinTitle" ) + L" " + LanguageProvider.GetDecodedString( "NewVerAva" ) + L" (" + std::wstring( NewVersion.begin( ), NewVersion.end( ) ) + L")" );
                        spdlog::info( "New version available: {}", NewVersion );
                        spdlog::info( "Your version: {}", WUWAOPT_VERSION );
                    } );
            } }
            .detach( );
    }

    OptimizerUIConfig UIConfig( LanguageProvider );
    UIConfig.LoadTexture( "Translate", "data/translate_icon.png" );
    UIConfig.LoadTexture( "Backpack", "data/backpack.png" );
    UIConfig.LoadTexture( "Settings", "data/settings.png" );
    UIConfig.LoadTexture( "Lock", "data/lock.png" );
    UIConfig.LoadTexture( "Unlock", "data/unlock.png" );

    for ( const auto& entry : std::filesystem::directory_iterator( "data/echo_img" ) )
    {
        if ( entry.is_regular_file( ) )
        {
            UIConfig.LoadTexture( entry.path( ).stem( ).string( ), entry.path( ).string( ) );
        }
    }

    for ( const auto& entry : std::filesystem::directory_iterator( "data/set_img" ) )
    {
        if ( entry.is_regular_file( ) )
        {
            UIConfig.LoadTexture( entry.path( ).stem( ).string( ), entry.path( ).string( ) );
        }
    }

    Backpack PlayerBackpack( LanguageProvider );
    PlayerBackpack.Set( FullStatsList );
    Opt.SetEchoes( PlayerBackpack.GetSelectedContent( ) );

    EchoConstraint Constraints( LanguageProvider );

    auto& Style = ImGui::GetStyle( );
    {
        ImGui::StyleColorsClassic( );
        Style.WindowBorderSize = 0.0f;
        Style.FramePadding.y   = 5.0f;
        Style.FrameBorderSize  = 1.0f;
    }

    std::array<std::array<PlotCombinationMeta, WuWaGA::ResultLength>, GARuntimeReport::MaxCombinationCount> ResultDisplayBuffer { };

    std::array<std::array<PlotCombinationMeta, WuWaGA::ResultLength * 2>, GARuntimeReport::MaxCombinationCount> GroupedDisplayBuffer { };
    std::array<int, GARuntimeReport::MaxCombinationCount>                                                       CombinationLegendIndex { };

    EchoPotential        SelectedEchoPotential;
    CombinationMetaCache SelectedStatsCache { Opt.GetEffectiveEchos( ) };
    CombinationMetaCache HoverStatsCache { Opt.GetEffectiveEchos( ) };

    const auto DisplayCombination =
        [ & ]( CombinationMetaCache& MainDisplayStats ) {
            const bool ShowDifferent = SelectedStatsCache != MainDisplayStats;

            const auto DisplayRow = [ ShowDifferent ]( const char* Label, FloatTy OldValue, FloatTy Value, FloatTy Payoff = 0, FloatTy PayoffPerc = 0 ) {
                ImGui::TableNextRow( );

                if ( Payoff != 0 )
                    ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32( ImVec4( 0.2f + PayoffPerc * 0.6f, 0.2f, 0.2f, 0.65f ) ) );

                if ( ShowDifferent )
                {
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
                    if ( Payoff != 0 )
                    {
                        ImGui::TableSetColumnIndex( 2 );
                        ImGui::Text( "+%.3f", Payoff );
                    }
                } else
                {
                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Text( "%s", Label );
                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::Text( "%.3f", Value );
                    if ( Payoff != 0 )
                    {
                        ImGui::TableSetColumnIndex( 2 );
                        ImGui::Text( "+%.3f", Payoff );
                    }
                }
            };

            const auto DisplayRowHelper = [ & ]( const char* Label, auto& OldStat, auto& Stat, auto& PayoffStat, auto& PayoffPercStat, auto StatPtr ) {
                DisplayRow( Label, OldStat.*StatPtr, Stat.*StatPtr, PayoffStat.*StatPtr, PayoffPercStat.*StatPtr );
            };

            ImGui::SeparatorText( LanguageProvider[ "EffectiveStats" ] );
            if ( MainDisplayStats.IsValid( ) && ImGui::BeginTable( "EffectiveStats", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame ) )
            {
                ImGui::TableSetupColumn( LanguageProvider[ "Stat" ] );
                ImGui::TableSetupColumn( LanguageProvider[ "Number" ] );
                ImGui::TableSetupColumn( LanguageProvider[ "ImprovePerColumn" ] );
                ImGui::TableHeadersRow( );

                auto& SelectedStats     = SelectedStatsCache.GetDisplayStats( );
                auto& DisplayStats      = MainDisplayStats.GetDisplayStats( );
                auto& PayoffStats       = MainDisplayStats.GetIncreasePayOff( );
                auto& PayoffWeightStats = MainDisplayStats.GetIncreasePayOffWeight( );
                DisplayRowHelper( LanguageProvider[ "FlatAttack" ], SelectedStats, DisplayStats, PayoffStats, PayoffWeightStats, &EffectiveStats::flat_attack );
                DisplayRowHelper( LanguageProvider[ "Regen%" ], SelectedStats, DisplayStats, PayoffStats, PayoffWeightStats, &EffectiveStats::regen );
                DisplayRowHelper( LanguageProvider[ "Attack%" ], SelectedStats, DisplayStats, PayoffStats, PayoffWeightStats, &EffectiveStats::percentage_attack );

                DisplayRowHelper( LanguageProvider[ "ElementBuff%" ], SelectedStats, DisplayStats, PayoffStats, PayoffWeightStats, &EffectiveStats::buff_multiplier );
                DisplayRowHelper( LanguageProvider[ "AutoAttack%" ], SelectedStats, DisplayStats, PayoffStats, PayoffWeightStats, &EffectiveStats::auto_attack_buff );
                DisplayRowHelper( LanguageProvider[ "HeavyAttack%" ], SelectedStats, DisplayStats, PayoffStats, PayoffWeightStats, &EffectiveStats::heavy_attack_buff );
                DisplayRowHelper( LanguageProvider[ "SkillDamage%" ], SelectedStats, DisplayStats, PayoffStats, PayoffWeightStats, &EffectiveStats::skill_buff );
                DisplayRowHelper( LanguageProvider[ "UltDamage%" ], SelectedStats, DisplayStats, PayoffStats, PayoffWeightStats, &EffectiveStats::ult_buff );

                DisplayRowHelper( LanguageProvider[ "CritRate" ], SelectedStats, DisplayStats, PayoffStats, PayoffWeightStats, &EffectiveStats::crit_rate );
                DisplayRowHelper( LanguageProvider[ "CritDamage" ], SelectedStats, DisplayStats, PayoffStats, PayoffWeightStats, &EffectiveStats::crit_damage );
                DisplayRow( LanguageProvider[ "FinalAttack" ], SelectedStatsCache.GetFinalAttack( ), MainDisplayStats.GetFinalAttack( ) );

                DisplayRow( LanguageProvider[ "AlignedNonCritDamage" ], SelectedStatsCache.GetNormalDamage( ), MainDisplayStats.GetNormalDamage( ) );
                DisplayRow( LanguageProvider[ "AlignedCritDamage" ], SelectedStatsCache.GetCritDamage( ), MainDisplayStats.GetCritDamage( ) );
                DisplayRow( LanguageProvider[ "AlignedExpectedDamage" ], SelectedStatsCache.GetExpectedDamage( ), MainDisplayStats.GetExpectedDamage( ) );

                ImGui::EndTable( );
            }

            ImGui::SeparatorText( LanguageProvider[ "Combination" ] );

            if ( MainDisplayStats.IsValid( ) && ImGui::BeginTable( "EffectiveStats", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame ) )
            {
                for ( int i = 0; i < MainDisplayStats.GetSlotCount( ); ++i )
                {
                    ImGui::TableNextRow( );
                    ImGui::TableSetColumnIndex( 0 );

                    if ( MainDisplayStats.GetEdDropPercentageWithoutAt( i ) != 0 )
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32( ImVec4( 0.2f + MainDisplayStats.GetEdDropPercentageWithoutAt( i ) * 0.6f, 0.2f, 0.2f, 0.65f ) ) );

                    const auto& SelectedEcho = MainDisplayStats.GetFullEchoAtSlot( i );
                    // ImGui::Text( "%s", std::format( "{:=^54}", Index ).c_str( ) );

                    ImGui::Text( "%s", std::format( "{:8}:", LanguageProvider[ "EchoSet" ] ).c_str( ) );
                    ImGui::SameLine( );
                    ImGui::PushStyleColor( ImGuiCol_Text, EchoSetColor[ SelectedEcho.Set ] );
                    ImGui::Text( "%s", std::format( "{:26}", LanguageProvider[ SelectedEcho.GetSetName( ) ] ).c_str( ) );
                    ImGui::PopStyleColor( );
                    ImGui::SameLine( );
                    ImGui::Text( "%s", SelectedEcho.BriefStat( LanguageProvider ).c_str( ) );
                    ImGui::Text( "%s", SelectedEcho.DetailStat( LanguageProvider ).c_str( ) );
                }

                ImGui::EndTable( );
            }
        };

    std::vector<PlotCombinationMeta> TopCombination;

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
        UIConfig.PushFont( );

        static ImGuiWindowFlags flags    = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
        const ImGuiViewport*    viewport = ImGui::GetMainViewport( );
        ImGui::SetNextWindowPos( viewport->WorkPos );
        ImGui::SetNextWindowSize( viewport->WorkSize );
        if ( ImGui::Begin( "Display", nullptr, flags ) )
        {
            {
                ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 5.0f );
                ImGui::BeginChild( "GAStats", ImVec2( ChartSplitWidth - Style.WindowPadding.x, -1 ), ImGuiChildFlags_Border );

                ImGui::ProgressBar( std::ranges::fold_left( GAReport.MutationProb, 0.f, []( auto A, auto B ) {
                                        return A + ( B <= 0 ? 1 : B );
                                    } ) / GARuntimeReport::MaxCombinationCount,
                                    ImVec2( -1.0f, 0.0f ) );
                if ( ImPlot::BeginPlot( LanguageProvider[ "Overview" ], ImVec2( -1, 0 ), ImPlotFlags_NoLegend ) )
                {
                    // Labels and positions
                    static const double  positions[]    = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
                    static const FloatTy positionsn05[] = { -0.1, 0.9, 1.9, 2.9, 3.9, 4.9, 5.9, 6.9, 7.9, 8.9, 9.9 };
                    static const FloatTy positionsp05[] = { 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1, 10.1 };

                    // Setup
                    ImPlot::SetupLegend( ImPlotLocation_South, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside );
                    ImPlot::SetupAxes( LanguageProvider[ "Type" ], LanguageProvider[ "OptimalValue" ], ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit );

                    // Set axis ticks
                    ImPlot::SetupAxisTicks( ImAxis_X1, positions, GARuntimeReport::MaxCombinationCount, WuWaGA::CombinationLabels.data( ) );
                    ImPlot::SetupAxisLimits( ImAxis_X1, -1, 11, ImPlotCond_Always );

                    ImPlot::SetupAxis( ImAxis_X2, LanguageProvider[ "Type" ], ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoDecorations );
                    ImPlot::SetupAxisLimits( ImAxis_X2, -1, 11, ImPlotCond_Always );

                    ImPlot::SetupAxis( ImAxis_Y2, LanguageProvider[ "Progress" ], ImPlotAxisFlags_AuxDefault );
                    ImPlot::SetupAxisLimits( ImAxis_Y2, 0, 1, ImPlotCond_Always );

                    // Plot
                    ImPlot::SetAxes( ImAxis_X1, ImAxis_Y1 );
                    ImPlot::PlotBars( LanguageProvider[ "OptimalValue" ], positionsn05, GAReport.OptimalValue.data( ), GARuntimeReport::MaxCombinationCount, 0.2 );

                    ImPlot::SetAxes( ImAxis_X2, ImAxis_Y2 );
                    ImPlot::PlotBars( "Progress", positionsp05, GAReport.MutationProb.data( ), GARuntimeReport::MaxCombinationCount, 0.2 );

                    ImPlot::EndPlot( );
                }

                TopCombination.clear( );
                if ( ImPlot::BeginPlot( std::vformat( LanguageProvider[ "TopType" ], std::make_format_args( WuWaGA::ResultLength ) ).c_str( ) ) )
                {
                    ImPlot::SetupLegend( ImPlotLocation_South, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside );
                    ImPlot::SetupAxes( LanguageProvider[ "Rank" ], LanguageProvider[ "OptimalValue" ], ImPlotAxisFlags_LockMin, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit );
                    ImPlot::SetupAxesLimits( 0, WuWaGA::ResultLength - 1, 0, 1, ImPlotCond_Once );
                    ImPlot::SetupAxisZoomConstraints( ImAxis_X1, 0, WuWaGA::ResultLength - 1 );

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
                                                         return PlotCombinationMeta { .Value           = std::max( Record.Value, (FloatTy) 0 ) * StaticStatMultiplier,
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

                            ImPlot::PlotLineG( WuWaGA::CombinationLabels[ i ], PlotCombinationMeta::PlotCombinationMetaImPlotGetter, ResultDisplayBuffer[ i ].data( ), WuWaGA::ResultLength );

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

                        int Rank  = std::clamp( (int) std::round( mouse.x ), 0, WuWaGA::ResultLength - 1 );
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

                        // Only allow hover/selection for valid result
                        if ( ResultDisplayBuffer[ ClosestCombination ][ Rank ].Value > 0 )
                        {
                            auto& SelectedResult = ResultDisplayBuffer[ ClosestCombination ][ Rank ];

                            ImPlot::PushPlotClipRect( );
                            draw_list->AddCircleFilled( ImPlot::PlotToPixels( (float) Rank, SelectedResult.Value ), 5, IM_COL32( 255, 0, 0, 255 ) );
                            ImPlot::PopPlotClipRect( );

                            ImGui::BeginTooltip( );
                            HoverStatsCache.SetAsCombination( PlayerBackpack,
                                                              GetCommonStat( ),
                                                              OConfig.SelectedElement,
                                                              SelectedResult,
                                                              OConfig );
                            DisplayCombination( HoverStatsCache );
                            ImGui::EndTooltip( );

                            // Select the echo combination
                            if ( sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
                            {
                                SelectedStatsCache.SetAsCombination( PlayerBackpack,
                                                                     GetCommonStat( ),
                                                                     OConfig.SelectedElement,
                                                                     SelectedResult,
                                                                     OConfig );
                            }
                        }
                    }

                    ImPlot::EndPlot( );
                }

                if ( ImPlot::BeginPlot( std::vformat( LanguageProvider[ "Top" ], std::make_format_args( WuWaGA::ResultLength ) ).c_str( ) ) )
                {
                    ImPlot::SetupLegend( ImPlotLocation_South, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside );
                    ImPlot::SetupAxes( LanguageProvider[ "Rank" ], LanguageProvider[ "OptimalValue" ], ImPlotAxisFlags_LockMin, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit );
                    ImPlot::SetupAxesLimits( 0, WuWaGA::ResultLength - 1, 0, 1, ImPlotCond_Once );
                    ImPlot::SetupAxisZoomConstraints( ImAxis_X1, 0, WuWaGA::ResultLength - 1 );

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
                                    CurrentBuffer[ j * 2 ] = CurrentBuffer[ j * 2 + 1 ] = PlotCombinationMeta { NAN };
                                }
                            }

                            CombinationLegendIndex[ i ] = CombinationIndex;
                            if ( HasData ) CombinationIndex++;
                            ImPlot::PlotLineG( WuWaGA::CombinationLabels[ i ], PlotCombinationMeta::PlotCombinationMetaSegmentImPlotGetter, CurrentBuffer.data( ), WuWaGA::ResultLength * 2, ImPlotLineFlags_Segments | ( HasData ? 0 : ImPlotItemFlags_NoLegend ) );
                        }

                        ImDrawList* draw_list = ImPlot::GetPlotDrawList( );
                        if ( ImPlot::IsPlotHovered( ) )
                        {
                            ImPlotPoint mouse = ImPlot::GetPlotMousePos( );
                            int         Rank  = std::clamp( (int) std::round( mouse.x ), 0, WuWaGA::ResultLength - 1 );

                            auto& SelectedResult     = TopCombination[ Rank ];
                            int   ClosestCombination = SelectedResult.CombinationID;
                            int   ClosestRank        = SelectedResult.CombinationRank;

                            if ( ImPlot::GetCurrentContext( )->CurrentItems->GetLegendItem( CombinationLegendIndex[ ClosestCombination ] )->Show )
                            {
                                ImPlot::PushPlotClipRect( );
                                draw_list->AddCircleFilled( ImPlot::PlotToPixels( (float) Rank, SelectedResult.Value ), 5, IM_COL32( 255, 0, 0, 255 ) );
                                ImPlot::PopPlotClipRect( );

                                ImGui::BeginTooltip( );
                                HoverStatsCache.SetAsCombination( PlayerBackpack,
                                                                  GetCommonStat( ),
                                                                  OConfig.SelectedElement,
                                                                  ResultDisplayBuffer[ ClosestCombination ][ ClosestRank ],
                                                                  OConfig );
                                DisplayCombination( HoverStatsCache );
                                ImGui::EndTooltip( );

                                // Select the echo combination
                                if ( sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
                                {
                                    SelectedStatsCache.SetAsCombination( PlayerBackpack,
                                                                         GetCommonStat( ),
                                                                         OConfig.SelectedElement,
                                                                         ResultDisplayBuffer[ ClosestCombination ][ ClosestRank ],
                                                                         OConfig );
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

                ImGui::BeginChild( "Right" );
                ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 5.0f );
                ImGui::BeginChild( "ConfigPanel", ImVec2( StatSplitWidth - Style.WindowPadding.x * 4, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize );

                ImGui::BeginChild( "ConfigPanel##Character", ImVec2( StatSplitWidth / 2 - Style.WindowPadding.x * 4, 0 ), ImGuiChildFlags_AutoResizeY );
                ImGui::SeparatorText( LanguageProvider[ "Character" ] );
                ImGui::PushID( "CharacterStat" );
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "FlatAttack" ], &OConfig.CharacterStats.flat_attack, 1, 0, 0, "%.0f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "Attack%" ], &OConfig.CharacterStats.percentage_attack, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "ElementBuff%" ], &OConfig.CharacterStats.buff_multiplier, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "AutoAttack%" ], &OConfig.CharacterStats.auto_attack_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "HeavyAttack%" ], &OConfig.CharacterStats.heavy_attack_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "SkillDamage%" ], &OConfig.CharacterStats.skill_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "UltDamage%" ], &OConfig.CharacterStats.ult_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "CritRate" ], &OConfig.CharacterStats.crit_rate, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "CritDamage" ], &OConfig.CharacterStats.crit_damage, 0.01, 0, 0, "%.2f" ) )
                ImGui::PopID( );
                ImGui::EndChild( );
                ImGui::SameLine( );
                ImGui::BeginChild( "ConfigPanel##Weapon", ImVec2( StatSplitWidth / 2 - Style.WindowPadding.x * 4, 0 ), ImGuiChildFlags_AutoResizeY );
                ImGui::SeparatorText( LanguageProvider[ "Weapon" ] );
                ImGui::PushID( "WeaponStat" );
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "FlatAttack" ], &OConfig.WeaponStats.flat_attack, 1, 0, 0, "%.0f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "Attack%" ], &OConfig.WeaponStats.percentage_attack, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "ElementBuff%" ], &OConfig.WeaponStats.buff_multiplier, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "AutoAttack%" ], &OConfig.WeaponStats.auto_attack_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "HeavyAttack%" ], &OConfig.WeaponStats.heavy_attack_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "SkillDamage%" ], &OConfig.WeaponStats.skill_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "UltDamage%" ], &OConfig.WeaponStats.ult_buff, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "CritRate" ], &OConfig.WeaponStats.crit_rate, 0.01, 0, 0, "%.2f" ) )
                SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "CritDamage" ], &OConfig.WeaponStats.crit_damage, 0.01, 0, 0, "%.2f" ) )
                ImGui::PopID( );
                ImGui::EndChild( );

                ImGui::NewLine( );
                ImGui::Separator( );
                ImGui::NewLine( );

                ImGui::PushItemWidth( -200 );

#define SAVE_MULTIPLIER_CONFIG( name, stat )                                          \
    if ( ImGui::DragFloat( name, &OptimizeMultiplierDisplay.stat ) )                  \
    {                                                                                 \
        OConfig.OptimizeMultiplierConfig.stat = OptimizeMultiplierDisplay.stat / 100; \
        OConfig.SaveConfig( );                                                        \
    }
                SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "AutoTotal%" ], auto_attack_multiplier )
                SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "HeavyTotal%" ], heavy_attack_multiplier )
                SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "SkillTotal%" ], skill_multiplier )
                SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "UltTotal%" ], ult_multiplier )
#undef SAVE_MULTIPLIER_CONFIG

                ImGui::NewLine( );
                ImGui::Separator( );
                ImGui::NewLine( );

                SAVE_CONFIG( ImGui::Combo( LanguageProvider[ "ElementType" ], &OConfig.SelectedElement, ElementLabel.GetRawStrings( ), ElementLabel.GetStringCount( ) ) )

                ImGui::NewLine( );
                ImGui::Separator( );
                ImGui::NewLine( );

                const auto  OptRunning = Opt.IsRunning( );
                const float ButtonH    = OptRunning ? 0 : 0.384;
                const auto  ButtonText = OptRunning ? LanguageProvider[ "StopCal" ] : LanguageProvider[ "Run" ];
                const auto  TestSize   = ImGui::CalcTextSize( ButtonText );
                const auto  ButtonSize = ImVec2 { ImGui::GetWindowWidth( ) - Style.WindowPadding.x * 2 - Style.FramePadding.x * 4 - TestSize.y, TestSize.y + Style.FramePadding.y * 2 };
                ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) ImColor::HSV( ButtonH, 0.6f, 0.6f ) );
                ImGui::PushStyleColor( ImGuiCol_ButtonHovered, (ImVec4) ImColor::HSV( ButtonH, 0.7f, 0.7f ) );
                ImGui::PushStyleColor( ImGuiCol_ButtonActive, (ImVec4) ImColor::HSV( ButtonH, 0.8f, 0.8f ) );
                if ( ImGui::Button( ButtonText, ButtonSize ) )
                {
                    if ( OptRunning )
                    {
                        Opt.Stop( );
                    } else
                    {
                        const auto BaseAttack = OConfig.GetBaseAttack( );
                        OptimizerParmSwitcher::SwitchRun( Opt, OConfig.SelectedElement, BaseAttack, GetCommonStat( ), &OConfig.OptimizeMultiplierConfig, Constraints );
                    }
                }
                ImGui::PopStyleColor( 3 );
                ImGui::SameLine( );

                {
                    if ( ImGui::ImageButton( *UIConfig.GetTexture( "Settings" ), sf::Vector2f { TestSize.y, TestSize.y } ) )
                    {
                        ImGui::OpenPopup( LanguageProvider[ "Constraint" ] );
                    }

                    // Always center this window when appearing
                    ImVec2 center = viewport->GetCenter( );
                    ImGui::SetNextWindowPos( center, ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );

                    if ( ImGui::BeginPopupModal( LanguageProvider[ "Constraint" ], nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove ) )
                    {
                        ImGui::Text( LanguageProvider[ "ConstraintDes" ] );
                        ImGui::Separator( );

                        Constraints.DisplayConstraintMenu( );

                        if ( ImGui::Button( LanguageProvider[ "Done" ], ImVec2( -1, 0 ) ) )
                        {
                            ImGui::CloseCurrentPopup( );
                        }
                        ImGui::SetItemDefaultFocus( );
                        ImGui::EndPopup( );
                    }
                }

                ImGui::NewLine( );
                ImGui::SeparatorText( LanguageProvider[ "StaticConfig" ] );
                SAVE_CONFIG( ImGui::SliderInt( LanguageProvider[ "CharLevel" ], &OConfig.CharacterLevel, 1, 90 ) )
                SAVE_CONFIG( ImGui::SliderInt( LanguageProvider[ "EnemyLevel" ], &OConfig.EnemyLevel, 1, 90 ) )
                SAVE_CONFIG( ImGui::SliderFloat( LanguageProvider[ "ElResistance" ], &OConfig.ElementResistance, 0, 1, "%.2f" ) )
                SAVE_CONFIG( ImGui::SliderFloat( LanguageProvider[ "DamReduction" ], &OConfig.ElementDamageReduce, 0, 1, "%.2f" ) )

                ImGui::PopItemWidth( );

                const auto ConfigHeight = ImGui::GetWindowHeight( );
                ImGui::EndChild( );

                ImGui::SetNextWindowSizeConstraints( ImVec2 { -1, viewport->WorkSize.y - ConfigHeight - Style.WindowPadding.y * 2 - Style.FramePadding.y }, { -1, FLT_MAX } );
                ImGui::BeginChild( "DetailPanel", ImVec2( StatSplitWidth - Style.WindowPadding.x * 4, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY );
                if ( ImGui::BeginTabBar( "CombinationTweak" ) )
                {
                    if ( ImGui::TabItemButton( "+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip ) )
                    {
                        CombinationTweaks.emplace_back( LanguageProvider, SelectedStatsCache );
                    }

                    if ( ImGui::BeginTabItem( LanguageProvider[ "CombinationDetail" ] ) )
                    {
                        if ( SelectedStatsCache.IsValid( ) )
                        {
                            DisplayCombination( SelectedStatsCache );
                        }

                        ImGui::EndTabItem( );
                    }

                    // Submit our regular tabs
                    int  Index   = 0;
                    auto TweakIt = CombinationTweaks.begin( );
                    while ( TweakIt != CombinationTweaks.end( ) )
                    {
                        bool        KeepOpen = true;
                        std::string Label    = LanguageProvider[ "CombinationTweak" ];
                        Label += " #" + std::to_string( Index );
                        if ( ImGui::BeginTabItem( Label.c_str( ), &KeepOpen, ImGuiTabItemFlags_None ) )
                        {
                            TweakIt->TweakerMenu( EchoNameBySet );

                            ImGui::EndTabItem( );
                        }

                        if ( !KeepOpen )
                        {
                            TweakIt = CombinationTweaks.erase( TweakIt );
                        } else
                        {
                            ++TweakIt;
                            ++Index;
                        }
                    }

                    ImGui::EndTabBar( );
                }
                ImGui::EndChild( );

                ImGui::PopStyleVar( );
                ImGui::EndChild( );

#undef SAVE_CONFIG
            }
        }
        ImGui::End( );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2 { 5, 5 } );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2 { 0, 0 } );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0 );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0 );

        const sf::Vector2f SLIconSize = sf::Vector2f( 30, 30 );
        ImGui::SetNextWindowPos( ImVec2 { viewport->WorkSize.x - SLIconSize.x - 10, viewport->WorkPos.y }, ImGuiCond_Always );
        if ( ImGui::Begin( "Language", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove ) )
        {
            if ( ImGui::ImageButton( *UIConfig.GetTexture( "Translate" ), SLIconSize ) )
            {
                ImGui::OpenPopup( "LanguageSelect" );
            }
        }

        ImGui::PopStyleVar( 4 );

        if ( ImGui::BeginPopupContextWindow( "LanguageSelect", ImGuiPopupFlags_NoReopen ) )
        {
            if ( ImGui::MenuItem( "en-US" ) )
            {
                LanguageProvider.LoadLanguage( OConfig.LastUsedLanguage = Language::English );
                OConfig.SaveConfig( );
            }
            if ( ImGui::MenuItem( "zh-CN" ) )
            {
                LanguageProvider.LoadLanguage( OConfig.LastUsedLanguage = Language::SimplifiedChinese );
                OConfig.SaveConfig( );
            }

            ImGui::EndPopup( );
        }

        ImGui::End( );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2 { 5, 5 } );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2 { 0, 0 } );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0 );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0 );

        const auto OptRunning = Opt.IsRunning( );
        ImGui::SetNextWindowPos( ImVec2 { viewport->WorkSize.x - SLIconSize.x * 2 - 20, viewport->WorkPos.y }, ImGuiCond_Always );
        if ( ImGui::Begin( "BackpackIcon", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove ) )
        {
            if ( OptRunning ) ImGui::BeginDisabled( );
            if ( ImGui::ImageButton( *UIConfig.GetTexture( "Backpack" ), SLIconSize ) )
            {
                ImGui::OpenPopup( LanguageProvider[ "Backpack" ] );
            }
            if ( OptRunning ) ImGui::EndDisabled( );
        }

        ImGui::PopStyleVar( 4 );

        if ( !OptRunning )
        {
            if ( PlayerBackpack.DisplayBackpack( ) )
            {
                spdlog::info( "Backpack changes" );
                Opt.SetEchoes( PlayerBackpack.GetSelectedContent( ) );
            }
        }

        ImGui::End( );

        ImGui::PopFont( );

        window.clear( );
        ImGui::SFML::Render( window );
        window.display( );
    }

    ImPlot::DestroyContext( );
    ImGui::SFML::Shutdown( );

    return 0;
}