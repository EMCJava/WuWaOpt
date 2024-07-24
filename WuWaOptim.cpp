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

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <random>

#include "Opt/OptimizerParmSwitcher.hpp"
#include "Opt/FullStats.hpp"
#include "Opt/OptUtil.hpp"
#include "Opt/WuWaGa.hpp"

#include "Loca/Loca.hpp"

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

inline const char* GTypeLabel[] = { "444", "4431", "3333", "44111", "41111", "43311", "43111", "31111", "33111", "33311", "11111" };

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

    Language LastUsedLanguage = Language::Undefined;

    // + 1 when saved to file
    int InternalStateID = 0;

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
            OptimizerConfigFile.ignore( std::numeric_limits<std::streamsize>::max( ) );
            const auto FileLength = OptimizerConfigFile.gcount( );
            OptimizerConfigFile.clear( );   //  Since ignore will have set eof.
            OptimizerConfigFile.seekg( 0, std::ios_base::beg );

            auto ActualReadSize = sizeof( OptimizerConfig );
            if ( FileLength < ActualReadSize )
            {
                ActualReadSize = FileLength;
                spdlog::warn( "Data file version mismatch, reading all available data." );
            }

            OptimizerConfigFile.read( (char*) this, ActualReadSize );
        }
    }

    void SaveConfig( )
    {
        InternalStateID++;

        static_assert( std::is_trivially_copy_assignable_v<OptimizerConfig> );
        std::ofstream OptimizerConfigFile( "oc.data", std::ios::binary );
        if ( OptimizerConfigFile )
        {
            OptimizerConfigFile.write( (char*) this, sizeof( OptimizerConfig ) );
        }
    }

    [[nodiscard]] auto GetBaseAttack( ) const noexcept
    {
        return WeaponStats.flat_attack + CharacterStats.flat_attack;
    }
};

template <typename EffectiveEchoListTy>
struct DisplayStatsCache {
    explicit DisplayStatsCache( EffectiveEchoListTy& EchoList )
        : EchoList( EchoList )
    {
    }

    EffectiveEchoListTy& EchoList;

    int            CachedConfigStateID { };
    EffectiveStats ActualStats { };
    EffectiveStats DisplayStats { };
    EffectiveStats IncreasePayOff { };
    EffectiveStats IncreasePayOffWeight { };
    FloatTy        FinalAttack    = 0;
    FloatTy        NormalDamage   = 0;
    FloatTy        CritDamage     = 0;
    FloatTy        ExpectedDamage = 0;

    int CombinationIndex = 0;
    int Rank             = 0;
    int ElementOffset    = 0;
    int SlotCount        = 0;

    bool Valid = false;

    std::array<int, 5> EchoIndices;

    void Deactivate( )
    {
        Valid       = false;
        ActualStats = DisplayStats = IncreasePayOff = { };
    }

    void SetAsCombination( const EffectiveStats& CommonStats,
                           int EO, int CI, int R,
                           const auto&            ResultBuffer,
                           const OptimizerConfig& Config )
    {
        CombinationIndex = CI;
        Rank             = R;
        SlotCount        = (int) strlen( GTypeLabel[ CombinationIndex ] );

        const auto&        CombinationUsed = ResultBuffer[ CombinationIndex ][ Rank ];
        std::array<int, 5> NewEchoIndices { };
        for ( int i = 0; i < SlotCount; ++i )
            NewEchoIndices[ i ] = CombinationUsed.Indices[ i ];

        // Check if the combination has changed
        if ( Valid && ElementOffset == EO && CachedConfigStateID == Config.InternalStateID && std::ranges::equal( NewEchoIndices, EchoIndices ) ) return;
        EchoIndices         = NewEchoIndices;
        CachedConfigStateID = Config.InternalStateID;

        ElementOffset = EO;
        Valid         = true;

        ActualStats = OptimizerParmSwitcher::SwitchCalculateCombinationalStat(
            ElementOffset,
            std::views::iota( 0, SlotCount )
                | std::views::transform( [ & ]( int EchoIndex ) -> EffectiveStats {
                      return EchoList[ EchoIndices[ EchoIndex ] ];
                  } )
                | std::ranges::to<std::vector>( ),
            CommonStats );

        DisplayStats.flat_attack       = ActualStats.flat_attack;
        DisplayStats.regen             = ActualStats.regen * 100 + 100;
        DisplayStats.percentage_attack = ActualStats.percentage_attack * 100;
        DisplayStats.buff_multiplier   = ActualStats.buff_multiplier * 100;
        DisplayStats.auto_attack_buff  = ActualStats.auto_attack_buff * 100;
        DisplayStats.heavy_attack_buff = ActualStats.heavy_attack_buff * 100;
        DisplayStats.skill_buff        = ActualStats.skill_buff * 100;
        DisplayStats.ult_buff          = ActualStats.ult_buff * 100;
        DisplayStats.crit_rate         = ActualStats.CritRateStat( ) * 100;
        DisplayStats.crit_damage       = ActualStats.CritDamageStat( ) * 100;
        FinalAttack                    = ActualStats.AttackStat( Config.GetBaseAttack( ) );

        CalculateDamages( Config );
    }

    void CalculateDamages( const OptimizerConfig& Config )
    {
        const FloatTy Resistances = Config.GetResistances( );
        const FloatTy BaseAttack  = Config.GetBaseAttack( );

        ActualStats.ExpectedDamage( BaseAttack, &Config.OptimizeMultiplierConfig,
                                    NormalDamage,
                                    CritDamage,
                                    ExpectedDamage );

        static constexpr std::array<FloatTy EffectiveStats::*, 9> PercentageStats {
            &EffectiveStats::regen,
            &EffectiveStats::percentage_attack,
            &EffectiveStats::buff_multiplier,
            &EffectiveStats::crit_rate,
            &EffectiveStats::crit_damage,
            &EffectiveStats::auto_attack_buff,
            &EffectiveStats::heavy_attack_buff,
            &EffectiveStats::skill_buff,
            &EffectiveStats::ult_buff,
        };

        FloatTy MaxDamageBuff = 0;

        {
            auto NewStat = ActualStats;
            NewStat.flat_attack += 1;

            const auto NewExpDmg = NewStat.ExpectedDamage( BaseAttack, &Config.OptimizeMultiplierConfig );
            MaxDamageBuff        = std::max( IncreasePayOff.flat_attack = NewExpDmg - ExpectedDamage, MaxDamageBuff );
        }

        for ( auto StatSlot : PercentageStats )
        {
            auto NewStat = ActualStats;
            NewStat.*StatSlot += 0.01f;

            const auto NewExpDmg = NewStat.ExpectedDamage( BaseAttack, &Config.OptimizeMultiplierConfig );
            MaxDamageBuff        = std::max( IncreasePayOff.*StatSlot = NewExpDmg - ExpectedDamage, MaxDamageBuff );
        }

        IncreasePayOffWeight.flat_attack = IncreasePayOff.flat_attack / MaxDamageBuff;
        for ( auto StatSlot : PercentageStats )
            IncreasePayOffWeight.*StatSlot = IncreasePayOff.*StatSlot / MaxDamageBuff;

        NormalDamage *= Resistances;
        CritDamage *= Resistances;
        ExpectedDamage *= Resistances;
    }

    bool operator==( const DisplayStatsCache& Other ) const noexcept
    {
        return Valid == Other.Valid && CombinationIndex == Other.CombinationIndex && Rank == Other.Rank && ElementOffset == Other.ElementOffset;
    }

    bool operator!=( const DisplayStatsCache& Other ) const noexcept
    {
        return !( *this == Other );
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
    OptimizerConfig  OConfig;
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

    const char* ElementLabel[] = {
        LanguageProvider[ "FireDamage" ],
        LanguageProvider[ "AirDamage" ],
        LanguageProvider[ "IceDamage" ],
        LanguageProvider[ "ElectricDamage" ],
        LanguageProvider[ "DarkDamage" ],
        LanguageProvider[ "LightDamage" ],
    };

    WuWaGA Opt( FullStatsList );

    constexpr auto   ChartSplitWidth = 700;
    constexpr auto   StatSplitWidth  = 800;
    sf::RenderWindow window( sf::VideoMode( ChartSplitWidth + StatSplitWidth, 1000 ), LanguageProvider.GetDecodedString( "WinTitle" ) );
    window.setFramerateLimit( 60 );
    if ( !ImGui::SFML::Init( window, false ) ) return -1;
    ImPlot::CreateContext( );

    ImGuiIO& io          = ImGui::GetIO( );
    auto*    EnglishFont = io.Fonts->AddFontDefault( );
    auto*    ChineseFont = io.Fonts->AddFontFromFileTTF( "data/sim_chi.ttf", 15.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull( ) );
    ImGui::SFML::UpdateFontTexture( );

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

    DisplayStatsCache SelectedStatsCache { Opt.GetEffectiveEchos( ) };
    DisplayStatsCache HoverStatsCache { Opt.GetEffectiveEchos( ) };

    const auto DisplayCombination =
        [ & ]<typename Ty>( DisplayStatsCache<Ty>& MainDisplayStats ) {
            const bool ShowDifferent = SelectedStatsCache != MainDisplayStats;

            const auto DisplayRow = [ ShowDifferent ]( const char* Label, FloatTy OldValue, FloatTy Value, FloatTy Payoff = 0, FloatTy PayoffPerc = 0 ) {
                ImGui::TableNextRow( );

                if ( Payoff != 0 )
                    ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32( ImVec4( PayoffPerc * 0.8f, 0.2f, 0.2f, 0.65f ) ) );

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
            if ( ImGui::BeginTable( "EffectiveStats", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg ) )
            {
                ImGui::TableSetupColumn( LanguageProvider[ "Stat" ] );
                ImGui::TableSetupColumn( LanguageProvider[ "Number" ] );
                ImGui::TableSetupColumn( LanguageProvider[ "ImprovePerColumn" ] );
                ImGui::TableHeadersRow( );

                auto& SelectedStats     = SelectedStatsCache.DisplayStats;
                auto& DisplayStats      = MainDisplayStats.DisplayStats;
                auto& PayoffStats       = MainDisplayStats.IncreasePayOff;
                auto& PayoffWeightStats = MainDisplayStats.IncreasePayOffWeight;
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
                DisplayRow( LanguageProvider[ "FinalAttack" ], SelectedStatsCache.FinalAttack, MainDisplayStats.FinalAttack );

                DisplayRow( LanguageProvider[ "AlignedNonCritDamage" ], SelectedStatsCache.NormalDamage, MainDisplayStats.NormalDamage );
                DisplayRow( LanguageProvider[ "AlignedCritDamage" ], SelectedStatsCache.CritDamage, MainDisplayStats.CritDamage );
                DisplayRow( LanguageProvider[ "AlignedExpectedDamage" ], SelectedStatsCache.ExpectedDamage, MainDisplayStats.ExpectedDamage );

                ImGui::EndTable( );
            }

            ImGui::SeparatorText( LanguageProvider[ "Combination" ] );
            for ( int i = 0; i < MainDisplayStats.SlotCount; ++i )
            {
                const auto  Index        = MainDisplayStats.EchoIndices[ i ];
                const auto& SelectedEcho = FullStatsList[ Index ];
                ImGui::Text( "%s", std::format( "{:=^54}", Index ).c_str( ) );

                ImGui::Text( "%s", std::format( "{:8}:", LanguageProvider[ "EchoSet" ] ).c_str( ) );
                ImGui::SameLine( );
                ImGui::PushStyleColor( ImGuiCol_Text, EchoSetColor[ SelectedEcho.Set ] );
                ImGui::Text( "%s", std::format( "{:26}", LanguageProvider[ SelectedEcho.GetSetName( ) ] ).c_str( ) );
                ImGui::PopStyleColor( );
                ImGui::SameLine( );
                ImGui::Text( "%s", SelectedEcho.BriefStat( LanguageProvider ).c_str( ) );
                ImGui::Text( "%s", SelectedEcho.DetailStat( LanguageProvider ).c_str( ) );
            }
        };

    std::vector<ResultPlotDetails> TopCombination;

    sf::Texture LanguageTexture;
    LanguageTexture.loadFromFile( "data/translate_icon.png" );

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

        switch ( LanguageProvider.GetLanguage( ) )
        {
        case Language::English: ImGui::PushFont( EnglishFont ); break;
        case Language::SimplifiedChinese: ImGui::PushFont( ChineseFont ); break;
        default: ImGui::PushFont( EnglishFont );
        }

        static ImGuiWindowFlags flags    = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
        const ImGuiViewport*    viewport = ImGui::GetMainViewport( );
        ImGui::SetNextWindowPos( viewport->WorkPos );
        ImGui::SetNextWindowSize( viewport->WorkSize );
        if ( ImGui::Begin( "Display", nullptr, flags ) )
        {
            {
                ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 5.0f );
                ImGui::BeginChild( "GAStats", ImVec2( ChartSplitWidth - ImGui::GetStyle( ).WindowPadding.x, -1 ), ImGuiChildFlags_Border );

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
                    ImPlot::SetupAxisTicks( ImAxis_X1, positions, GARuntimeReport::MaxCombinationCount, GTypeLabel );
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
                    ImPlot::SetupAxesLimits( 0, WuWaGA::ResultLength, 0, 0, ImPlotCond_Once );

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

                            ImPlot::PlotLineG( GTypeLabel[ i ], ResultPlotDetailsImPlotGetter, ResultDisplayBuffer[ i ].data( ), WuWaGA::ResultLength );

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
                        HoverStatsCache.SetAsCombination( GetCommonStat( ),
                                                          OConfig.SelectedElement,
                                                          ClosestCombination, Rank,
                                                          ResultDisplayBuffer,
                                                          OConfig );
                        DisplayCombination( HoverStatsCache );
                        ImGui::EndTooltip( );

                        // Select the echo combination
                        if ( sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
                        {
                            SelectedStatsCache.SetAsCombination( GetCommonStat( ),
                                                                 OConfig.SelectedElement,
                                                                 ClosestCombination,
                                                                 Rank,
                                                                 ResultDisplayBuffer,
                                                                 OConfig );
                        }
                    }

                    ImPlot::EndPlot( );
                }

                if ( ImPlot::BeginPlot( std::vformat( LanguageProvider[ "Top" ], std::make_format_args( WuWaGA::ResultLength ) ).c_str( ) ) )
                {
                    ImPlot::SetupLegend( ImPlotLocation_South, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside );
                    ImPlot::SetupAxes( LanguageProvider[ "Rank" ], LanguageProvider[ "OptimalValue" ], ImPlotAxisFlags_LockMin, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit );
                    ImPlot::SetupAxesLimits( 0, WuWaGA::ResultLength, 0, 0, ImPlotCond_Once );

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
                            ImPlot::PlotLineG( GTypeLabel[ i ], ResultSegmentPlotDetailsImPlotGetter, CurrentBuffer.data( ), WuWaGA::ResultLength * 2, ImPlotLineFlags_Segments | ( HasData ? 0 : ImPlotItemFlags_NoLegend ) );
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
                                HoverStatsCache.SetAsCombination( GetCommonStat( ),
                                                                  OConfig.SelectedElement,
                                                                  ClosestCombination, ClosestRank,
                                                                  ResultDisplayBuffer,
                                                                  OConfig );
                                DisplayCombination( HoverStatsCache );
                                ImGui::EndTooltip( );

                                // Select the echo combination
                                if ( sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
                                {
                                    SelectedStatsCache.SetAsCombination( GetCommonStat( ),
                                                                         OConfig.SelectedElement,
                                                                         ClosestCombination,
                                                                         ClosestRank,
                                                                         ResultDisplayBuffer,
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

                ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 5.0f );
                ImGui::BeginChild( "DetailPanel", ImVec2( StatSplitWidth - ImGui::GetStyle( ).WindowPadding.x * 2, -1 ), ImGuiChildFlags_Border );

                ImGui::BeginChild( "DetailPanel##Character", ImVec2( StatSplitWidth / 2 - ImGui::GetStyle( ).WindowPadding.x * 4, 0 ), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize );
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
                ImGui::BeginChild( "DetailPanel##Weapon", ImVec2( StatSplitWidth / 2 - ImGui::GetStyle( ).WindowPadding.x * 4, 0 ), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize );
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

                SAVE_CONFIG( ImGui::Combo( LanguageProvider[ "ElementType" ], &OConfig.SelectedElement, ElementLabel, IM_ARRAYSIZE( ElementLabel ) ) )

                ImGui::NewLine( );
                ImGui::Separator( );
                ImGui::NewLine( );

                const auto  OptRunning = Opt.IsRunning( );
                const float ButtonH    = OptRunning ? 0 : 0.384;
                ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) ImColor::HSV( ButtonH, 0.6f, 0.6f ) );
                ImGui::PushStyleColor( ImGuiCol_ButtonHovered, (ImVec4) ImColor::HSV( ButtonH, 0.7f, 0.7f ) );
                ImGui::PushStyleColor( ImGuiCol_ButtonActive, (ImVec4) ImColor::HSV( ButtonH, 0.8f, 0.8f ) );
                if ( ImGui::Button( OptRunning ? LanguageProvider[ "ReRun" ] : LanguageProvider[ "Run" ], ImVec2( -1, 0 ) ) )
                {
                    const auto BaseAttack = OConfig.GetBaseAttack( );
                    OptimizerParmSwitcher::SwitchRun( Opt, OConfig.SelectedElement, BaseAttack, GetCommonStat( ), &OConfig.OptimizeMultiplierConfig );
                }
                ImGui::PopStyleColor( 3 );

                ImGui::NewLine( );
                ImGui::SeparatorText( LanguageProvider[ "StaticConfig" ] );
                SAVE_CONFIG( ImGui::SliderInt( LanguageProvider[ "CharLevel" ], &OConfig.CharacterLevel, 1, 90 ) )
                SAVE_CONFIG( ImGui::SliderInt( LanguageProvider[ "EnemyLevel" ], &OConfig.EnemyLevel, 1, 90 ) )
                SAVE_CONFIG( ImGui::SliderFloat( LanguageProvider[ "ElResistance" ], &OConfig.ElementResistance, 0, 1, "%.2f" ) )
                SAVE_CONFIG( ImGui::SliderFloat( LanguageProvider[ "DamReduction" ], &OConfig.ElementDamageReduce, 0, 1, "%.2f" ) )

                if ( SelectedStatsCache.Valid )
                {
                    DisplayCombination( SelectedStatsCache );
                }

                ImGui::EndChild( );
                ImGui::PopStyleVar( );

#undef SAVE_CONFIG
            }
        }
        ImGui::End( );

        const sf::Vector2f SLIconSize = sf::Vector2f( 30, 30 );
        ImGui::SetNextWindowPos( ImVec2 { viewport->WorkSize.x - SLIconSize.x - 10, viewport->WorkPos.y }, ImGuiCond_Always );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2 { 5, 5 } );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2 { 0, 0 } );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0 );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0 );

        if ( ImGui::Begin( "Language", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove ) )
        {
            if ( ImGui::ImageButton( LanguageTexture, SLIconSize ) )
            {
                ImGui::OpenPopup( "LanguageSelect" );
            }
        }

        ImGui::PopStyleVar( 4 );

        if ( ImGui::BeginPopupContextWindow( "LanguageSelect", ImGuiPopupFlags_NoReopen ) )
        {
            bool Modified = false;
            if ( ImGui::MenuItem( "en-US" ) )
            {
                LanguageProvider.LoadLanguage( OConfig.LastUsedLanguage = Language::English );
                Modified = true;
            }
            if ( ImGui::MenuItem( "zh-CN" ) )
            {
                LanguageProvider.LoadLanguage( OConfig.LastUsedLanguage = Language::SimplifiedChinese );
                Modified = true;
            }

            if ( Modified )
            {
                OConfig.SaveConfig( );

                std::ranges::copy( std::initializer_list<const char*> {
                                       LanguageProvider[ "FireDamage" ],
                                       LanguageProvider[ "AirDamage" ],
                                       LanguageProvider[ "IceDamage" ],
                                       LanguageProvider[ "ElectricDamage" ],
                                       LanguageProvider[ "DarkDamage" ],
                                       LanguageProvider[ "LightDamage" ],
                                   },
                                   ElementLabel );
            }

            ImGui::EndPopup( );
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