//
// Created by EMCJava on 7/26/2024.
//

#include "CombinationTweaker.hpp"

#include <Opt/UI/OptimizerUIConfig.hpp>

#include <Opt/OptUtil.hpp>

#include <magic_enum.hpp>

#include <imgui.h>
#include <imgui-SFML.h>

#include <format>

inline const char* CostNames[] = { "0", "1", "2", "3", "4" };

inline std::array<std::vector<StatValueConfig>, 5> MaxFirstMainStat {
    std::vector<StatValueConfig> { },
    std::vector<StatValueConfig> {
                                  StatValueConfig { nullptr, 0 },
                                  StatValueConfig { &EffectiveStats::percentage_attack, 0.18 } },
    std::vector<StatValueConfig> { },
    std::vector<StatValueConfig> {
                                  StatValueConfig { nullptr, 0 },
                                  StatValueConfig { &EffectiveStats::percentage_attack, 0.3 },
                                  StatValueConfig { &EffectiveStats::buff_multiplier, 0.3 } },
    std::vector<StatValueConfig> {
                                  StatValueConfig { nullptr, 0 },
                                  StatValueConfig { &EffectiveStats::percentage_attack, 0.33 },
                                  StatValueConfig { &EffectiveStats::crit_rate, 0.22 },
                                  StatValueConfig { &EffectiveStats::crit_damage, 0.44 },
                                  }
};

inline std::array<StatValueConfig, 5> MaxSecondMainStat {
    StatValueConfig { },
    StatValueConfig { },
    StatValueConfig { },
    StatValueConfig { &EffectiveStats::flat_attack, 100 },
    StatValueConfig { &EffectiveStats::flat_attack, 150 },
};


void
DisplayTextAt( auto&& Text, float Center )
{
    const auto SV          = std::string_view { Text };
    const auto WindowWidth = ImGui::GetWindowWidth( );
    const auto TextWidth   = ImGui::CalcTextSize( SV.data( ) ).x;
    ImGui::SetCursorPosX( WindowWidth * Center - TextWidth * 0.5f );
    ImGui::Text( "%s", SV.data( ) );
}

void
CombinationTweaker::TweakerMenu( const std::map<std::string, std::vector<std::string>>& EchoNamesBySet )
{
    if ( !m_TweakerTarget.IsValid( ) ) return;

    const auto& Style           = ImGui::GetStyle( );
    const auto  EchoConfigWidth = ImGui::GetWindowWidth( );
    ImGui::Dummy( ImVec2 { 0, 5 } );
    if ( ImGui::BeginTable( "SlotEffectiveness", m_TweakerTarget.GetSlotCount( ), ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp ) )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_SelectableTextAlign, ImVec2( 0.5f, 0.5f ) );
        ImGui::TableNextRow( );
        for ( int i = 0; i < m_TweakerTarget.GetSlotCount( ); ++i )
        {
            ImGui::TableSetColumnIndex( i );
            const auto EDDrop = m_TweakerTarget.GetEdDropPercentageWithoutAt( i );
            if ( EDDrop != 0 )
                ImGui::TableSetBgColor( ImGuiTableBgTarget_CellBg, ImGui::GetColorU32( ImVec4( 0.2f + EDDrop * 0.6f, 0.2f, 0.2f, 0.65f ) ) );

            const auto* EchoName = LanguageProvider[ m_TweakerTarget.GetEchoNameAtSlot( i ) ];

            ImGui::PushID( i );
            if ( ImGui::Selectable( EchoName, m_EchoSlotIndex == i ) )
            {
                m_EchoSlotIndex = m_EchoSlotIndex != i ? i : -1;
                ClearPotentialCache( );
            }
            ImGui::PopID( );
        }
        ImGui::PopStyleVar( );
        ImGui::EndTable( );
    }

    if ( m_EchoSlotIndex != -1 && m_EchoSlotIndex < m_TweakerTarget.GetSlotCount( ) )
    {
        auto CurrentTweakingCost = m_TweakerTarget.GetEffectiveEchoAtSlot( m_EchoSlotIndex ).Cost;
        if ( PreviousTweakingCost != CurrentTweakingCost )
        {
            CurrentTweakingCost = CurrentTweakingCost;
            m_MainStatLabel.SetKeyStrings(
                MaxFirstMainStat[ CurrentTweakingCost ]
                | std::views::transform( []( auto& Config ) {
                      return EffectiveStats::GetStatName( Config.ValuePtr );
                  } ) );
        }

        ImGui::SeparatorText( LanguageProvider[ "SubEchoCfg" ] );
        {
            ImGui::BeginDisabled( );
            ImGui::SetNextItemWidth( 50 );
            ImGui::Combo( "Cost",
                          &CurrentTweakingCost,
                          CostNames,
                          IM_ARRAYSIZE( CostNames ) );
            ImGui::EndDisabled( );
            ImGui::SameLine( );

            ImGui::SetNextItemWidth( 160 );
            if ( ImGui::Combo( LanguageProvider[ "EchoSet" ],
                               &m_SelectedEchoSet,
                               m_SetNames.GetRawStrings( ),
                               m_SetNames.GetStringCount( ) ) )
            {
                m_EchoNames.SetKeyStrings(
                    EchoNamesBySet.at( std::string( magic_enum::enum_name( static_cast<EchoSet>( m_SelectedEchoSet ) ) ) )
                    | std::views::transform( []( auto& Str ) {
                          return Str.c_str( );
                      } )
                    | std::ranges::to<std::vector>( ) );
                ClearPotentialCache( );
            }
            ImGui::SameLine( );

            ImGui::SetNextItemWidth( 160 );
            if ( ImGui::Combo( LanguageProvider[ "EchoName" ],
                               &m_SelectedEchoNameID,
                               m_EchoNames.GetRawStrings( ),
                               m_EchoNames.GetStringCount( ) ) )
            {
                ClearPotentialCache( );
            }

            ImGui::SameLine( );
            ImGui::SetNextItemWidth( 120 );
            if ( ImGui::Combo( LanguageProvider[ "MainStat" ],
                               &m_SelectedMainStatTypeIndex,
                               m_MainStatLabel.GetRawStrings( ),
                               m_MainStatLabel.GetStringCount( ) ) )
            {
                ClearPotentialCache( );
            }
        }

        ImGui::SeparatorText( LanguageProvider[ "SubStats" ] );
        {
            for ( int i = 0; i < m_UsedSubStatCount; ++i )
            {
                ImGui::PushID( i );
                ImGui::BeginChild( "EchoConfig", ImVec2( EchoConfigWidth - Style.WindowPadding.x * 2, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY );
                bool Changed = ImGui::Combo( LanguageProvider[ "SubStatType" ], &m_SelectedSubStatTypeIndices[ i ], m_SubStatLabel.GetRawStrings( ), m_SubStatLabel.GetStringCount( ) );

                ImGui::SameLine( );

                const auto SelectedTypeIt =
                    std::ranges::find_if( m_SubStatRollConfigs | std::views::reverse,
                                          [ Ptr = m_SubStatPtr[ m_SelectedSubStatTypeIndices[ i ] ] ]( const SubStatRollConfig& Config ) {
                                              return Config.ValuePtr == Ptr;
                                          } );
                if ( SelectedTypeIt == m_SubStatRollConfigs.rend( ) )
                {
                    ImGui::BeginDisabled( );
                    ImGui::Combo( LanguageProvider[ "SubStatValue" ],
                                  &m_SelectedSubStatValueIndices[ i ],
                                  SubStatRollConfig::GetValueString,
                                  nullptr, 0 );
                    ImGui::EndDisabled( );
                } else
                {
                    Changed |= ImGui::Combo( LanguageProvider[ "SubStatValue" ],
                                             &m_SelectedSubStatValueIndices[ i ],
                                             SubStatRollConfig::GetValueString,
                                             (void*) &*SelectedTypeIt,
                                             SelectedTypeIt->PossibleRolls );
                }

                ImGui::EndChild( );
                ImGui::PopID( );

                if ( Changed )
                {
                    if ( SelectedTypeIt != m_SubStatRollConfigs.rend( ) )
                    {
                        m_EchoSubStatConfigs[ i ] = {
                            .ValuePtr = SelectedTypeIt->ValuePtr,
                            .Value    = SelectedTypeIt->Values[ m_SelectedSubStatValueIndices[ i ] ].Value,
                        };
                    } else
                    {
                        m_EchoSubStatConfigs[ i ] = { };
                    }
                }
            }

            ImGui::PushID( m_UsedSubStatCount );
            ImGui::PushStyleVar( ImGuiStyleVar_SelectableTextAlign, ImVec2( 0.5f, 0.5f ) );

            const bool CantAdd = m_UsedSubStatCount >= MaxRollCount;
            if ( CantAdd ) ImGui::BeginDisabled( );
            if ( ImGui::Selectable( "+", true, ImGuiSelectableFlags_None, ImVec2( EchoConfigWidth / 2 - Style.WindowPadding.x * 2, 10 ) ) ) m_UsedSubStatCount++;
            if ( CantAdd ) ImGui::EndDisabled( );

            ImGui::SameLine( );

            const bool CantSub = m_UsedSubStatCount <= 0;
            if ( CantSub ) ImGui::BeginDisabled( );
            if ( ImGui::Selectable( "-", true, ImGuiSelectableFlags_None, ImVec2( EchoConfigWidth / 2 - Style.WindowPadding.x * 2, 10 ) ) ) m_UsedSubStatCount--;
            if ( CantSub ) ImGui::EndDisabled( );
            ImGui::PopStyleVar( );
            ImGui::PopID( );
        }

        ImGui::NewLine( );
        if ( ImGui::Button( LanguageProvider[ "Run" ], ImVec2( -1, 0 ) ) )
        {
            if ( MaxFirstMainStat[ CurrentTweakingCost ].size( ) > m_SelectedMainStatTypeIndex
                 && m_SelectedMainStatTypeIndex < MaxFirstMainStat[ CurrentTweakingCost ].size( )
                 && m_SelectedEchoSet != (int) EchoSet::eEchoSetNone
                 && m_SelectedEchoNameID < m_EchoNames.GetStringCount( ) )
            {
                EffectiveStats ConfiguredSubStats {
                    .Set    = (EchoSet) m_SelectedEchoSet,
                    .NameID = m_SelectedEchoNameID,
                    .Cost   = CurrentTweakingCost };

                for ( int i = 0; i < m_UsedSubStatCount; ++i )
                {
                    if ( m_EchoSubStatConfigs[ i ].ValuePtr != nullptr )
                    {
                        ConfiguredSubStats.*( m_EchoSubStatConfigs[ i ].ValuePtr ) += m_EchoSubStatConfigs[ i ].Value;
                    }
                }

                CalculateEchoPotential( m_SelectedEchoPotential,
                                        ConfiguredSubStats,
                                        MaxRollCount - m_UsedSubStatCount );
            } else
            {
                m_SelectedEchoPotential.CDF.clear( );
            }
        }
    }

    if ( !m_SelectedEchoPotential.CDF.empty( ) )
    {
        ImGui::SeparatorText( LanguageProvider[ "Result" ] );
        const auto SelectedEDIndex =
            std::distance(
                m_SelectedEchoPotential.CDFChangeToED.begin( ),
                std::ranges::lower_bound( m_SelectedEchoPotential.CDFChangeToED, (FloatTy) m_DragEDTargetY ) );

        if ( m_FullPotentialCache )
        {
            const auto SimpleViewWidth = EchoConfigWidth - Style.WindowPadding.x * 2;
            ImGui::BeginChild( "Yes/No", ImVec2( SimpleViewWidth, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY );
            ImGui::Spacing( );

            OptimizerUIConfig::PushFont( OptimizerUIConfig::Big );
            static constexpr std::array<int, 6> CumulativeEXP { 0, 4400, 16500, 39600, 79100, 142600 };

            const auto UsedEXP = CumulativeEXP[ m_UsedSubStatCount ];

            const auto RenewableEXP  = static_cast<decltype( UsedEXP )>( UsedEXP * 0.75 );
            const auto NewEchoEXPReq = CumulativeEXP.back( ) - RenewableEXP;

            const auto ImprovementPerEXPNew   = m_FullPotentialCache->ExpectedChangeToED / NewEchoEXPReq;
            const auto ImprovementPerEXPStick = m_SelectedEchoPotential.ExpectedChangeToED / ( CumulativeEXP.back( ) - UsedEXP );
            if ( ImprovementPerEXPStick >= ImprovementPerEXPNew )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 0, 255, 0, 255 ) );
                DisplayTextAt( LanguageProvider[ "KeepUG" ], 0.5 );
            } else
            {
                ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 255, 0, 0, 255 ) );
                DisplayTextAt( LanguageProvider[ "StopUG" ], 0.5 );
            }
            ImGui::PopStyleColor( );

            ImGui::PopFont( );

            ImGui::Spacing( );
            ImGui::EndChild( );
        }

        if ( ImGui::CollapsingHeader( LanguageProvider[ "KeyEDDis" ] ) )
        {
            const auto SimpleViewWidth = EchoConfigWidth - Style.WindowPadding.x * 2;
            ImGui::BeginChild( "SimpleView", ImVec2( SimpleViewWidth, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY );

            const auto DisplayNumberCompare =
                [ & ]( auto& OptionalLock, auto&& Value ) {
                    const float NumberFrameWidth = SimpleViewWidth * 0.4;
                    const float IconSize         = 30;

                    OptimizerUIConfig::PushFont( OptimizerUIConfig::Big );

                    if ( OptionalLock.has_value( ) )
                    {
                        bool ShouldResetValue = false;
                        ImGui::SetCursorPosX( ( SimpleViewWidth - NumberFrameWidth * 2 ) / 4 );
                        ImGui::BeginChild( (ImGuiID) &OptionalLock, ImVec2( NumberFrameWidth, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY );
                        ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.f, 0.f, 0.f, 0.f ) );

                        DisplayTextAt( std::format( "{:.3f} %", OptionalLock.value( ) ).c_str( ), 0.5 );
                        const auto LockIcon = OptimizerUIConfig::GetTextureOrDefault( "Lock" );
                        if ( LockIcon )
                        {
                            ImGui::SetCursorPos( ImVec2 { NumberFrameWidth - IconSize - Style.WindowPadding.x, 0 } );
                            if ( ImGui::ImageButton( *LockIcon, sf::Vector2f( IconSize, IconSize ) ) )
                                ShouldResetValue = true;
                        }

                        ImGui::PopStyleColor( 1 );
                        ImGui::EndChild( );

                        ImGui::SameLine( );
                        DisplayTextAt( "->", 0.5 );
                        ImGui::SameLine( );

                        ImGui::SetCursorPosX( SimpleViewWidth / 2 + ( SimpleViewWidth - NumberFrameWidth * 2 ) / 4 );
                        ImGui::BeginChild( (ImGuiID) &OptionalLock + 1, ImVec2( NumberFrameWidth, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY );
                        if ( Value - OptionalLock.value( ) > 0.001f )
                            ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 0, 255, 0, 255 ) );
                        else if ( Value - OptionalLock.value( ) < -0.001f )
                            ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 255, 0, 0, 255 ) );
                        else
                            ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32_WHITE );
                        DisplayTextAt( std::format( "{:.3f} %", Value ).c_str( ), 0.5 );
                        ImGui::PopStyleColor( );
                        ImGui::EndChild( );

                        if ( ShouldResetValue ) OptionalLock.reset( );
                    } else
                    {
                        ImGui::SetCursorPosX( ( SimpleViewWidth - NumberFrameWidth ) / 2 );
                        ImGui::BeginChild( (ImGuiID) &OptionalLock, ImVec2( NumberFrameWidth, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY );
                        ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.f, 0.f, 0.f, 0.f ) );

                        DisplayTextAt( std::format( "{:.3f} %", Value ).c_str( ), 0.5 );
                        const auto UnlockIcon = OptimizerUIConfig::GetTextureOrDefault( "Unlock" );
                        if ( UnlockIcon )
                        {
                            ImGui::SetCursorPos( ImVec2 { NumberFrameWidth - IconSize - Style.WindowPadding.x, 0 } );
                            if ( ImGui::ImageButton( *UnlockIcon, sf::Vector2f( IconSize, IconSize ) ) )
                                OptionalLock = Value;
                        }

                        ImGui::PopStyleColor( 1 );
                        ImGui::EndChild( );
                    }

                    ImGui::PopFont( );
                };

            ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.f, 0.f, 0.f, 0.f ) );
            ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.f, 0.f, 0.f, 0.f ) );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.f, 0.f, 0.f, 0.f ) );

            DisplayTextAt( std::vformat( LanguageProvider[ "ProbGetMoreImp" ], std::make_format_args( m_DragEDTargetY ) ), 0.5 );
            ImGui::NewLine( );
            DisplayNumberCompare( m_PinnedTargetCDF, m_SelectedEchoPotential.CDF[ SelectedEDIndex ] );
            ImGui::NewLine( );
            ImGui::Separator( );

            DisplayTextAt( LanguageProvider[ "EEDDelta%" ], 0.5 );
            ImGui::NewLine( );
            DisplayNumberCompare( m_PinnedExpectedChange, m_SelectedEchoPotential.ExpectedChangeToED );
            ImGui::NewLine( );

            ImGui::PopStyleColor( 3 );

            ImGui::EndChild( );
        }

        if ( ImGui::CollapsingHeader( LanguageProvider[ "PotentialEDDis" ] ) )
        {
            if ( ImGui::BeginTable( "PotentialStats", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame ) )
            {
                ImGui::TableSetupColumn( LanguageProvider[ "BestED" ] );
                ImGui::TableSetupColumn( LanguageProvider[ "Baseline" ] );
                ImGui::TableSetupColumn( LanguageProvider[ "WorstED" ] );
                ImGui::TableHeadersRow( );
                ImGui::TableNextRow( );

                ImGui::TableNextColumn( );
                ImGui::Text( m_SelectedEchoPotential.CDFChangeToED.back( ) > 0 ? "%.2f     (+%.2f%%)" : "%.2f     (%.2f%%)",
                             m_SelectedEchoPotential.HighestExpectedDamage,
                             m_SelectedEchoPotential.CDFChangeToED.back( ) );
                ImGui::TableNextColumn( );
                ImGui::Text( "%.2f", m_SelectedEchoPotential.BaselineExpectedDamage );
                ImGui::TableNextColumn( );
                ImGui::Text( m_SelectedEchoPotential.CDFChangeToED.front( ) > 0 ? "%.2f     (+%.2f%%)" : "%.2f     (%.2f%%)",
                             m_SelectedEchoPotential.LowestExpectedDamage,
                             m_SelectedEchoPotential.CDFChangeToED.front( ) );

                ImGui::EndTable( );
            }

            if ( ImPlot::BeginPlot( LanguageProvider[ "Potential" ], ImVec2( -1, 0 ) ) )
            {
                ImPlot::SetupLegend( ImPlotLocation_North | ImPlotLocation_West, ImPlotLegendFlags_Outside - 1 );
                ImPlot::SetupAxes( LanguageProvider[ "Probability%" ], LanguageProvider[ "EDDelta%" ], ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_Invert, ImPlotAxisFlags_AutoFit );
                ImPlot::SetupAxis( ImAxis_Y2, LanguageProvider[ "ExpectedDamage" ], ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_AutoFit );
                ImPlot::SetupAxisLimitsConstraints( ImAxis_Y2, m_SelectedEchoPotential.CDFSmallOrEqED.front( ), m_SelectedEchoPotential.CDFSmallOrEqED.back( ) );

                ImPlot::SetAxes( ImAxis_X1, ImAxis_Y2 );
                ImPlot::PlotLine( "ED_CDF",
                                  m_SelectedEchoPotential.CDFFloat.data( ),
                                  m_SelectedEchoPotential.CDFSmallOrEqED.data( ),
                                  m_SelectedEchoPotential.CDF.size( ), ImPlotItemFlags_NoLegend );

                ImPlot::SetAxes( ImAxis_X1, ImAxis_Y1 );
                ImPlot::PlotLine( "Delta%CDF",
                                  m_SelectedEchoPotential.CDFFloat.data( ),
                                  m_SelectedEchoPotential.CDFChangeToED.data( ),
                                  m_SelectedEchoPotential.CDF.size( ), ImPlotItemFlags_NoLegend );

                if ( m_SelectedEchoPotential.CDF.size( ) > SelectedEDIndex )
                {
                    ImPlot::SetNextFillStyle( ImVec4 { 0, 1, 0, 1 }, 0.2 );
                    ImPlot::PlotShaded( "Delta%CDFS",
                                        m_SelectedEchoPotential.CDFFloat.data( ) + SelectedEDIndex,
                                        m_SelectedEchoPotential.CDFChangeToED.data( ) + SelectedEDIndex,
                                        m_SelectedEchoPotential.CDF.size( ) - SelectedEDIndex, m_DragEDTargetY, ImPlotItemFlags_NoLegend );
                }

                ImPlot::TagX( m_SelectedEchoPotential.CDF[ SelectedEDIndex ], ImVec4( 0, 1, 0, 1 ), "%.2f%%", m_SelectedEchoPotential.CDF[ SelectedEDIndex ] );

                ImPlot::DragLineY( 1, &m_SelectedEchoPotential.ExpectedChangeToED, ImVec4( 1, 1, 0, 1 ), 1, ImPlotDragToolFlags_NoInputs | ImPlotDragToolFlags_NoFit );
                ImPlot::TagY( m_SelectedEchoPotential.ExpectedChangeToED, ImVec4( 1, 1, 0, 1 ), true );

                ImPlot::DragLineY( 0, &m_DragEDTargetY, ImVec4( 1, 0, 0, 1 ), 1, ImPlotDragToolFlags_Delayed );
                ImPlot::TagY( m_DragEDTargetY, ImVec4( 1, 0, 0, 1 ), true );
                m_DragEDTargetY = std::clamp( m_DragEDTargetY,
                                              (double) m_SelectedEchoPotential.CDFChangeToED.front( ),
                                              (double) m_SelectedEchoPotential.CDFChangeToED.back( ) );

                ImPlot::SetNextLineStyle( ImVec4( 1, 1, 0, 1 ) );
                ImPlot::PlotDummy( LanguageProvider[ "EEDDelta%" ] );
                // ImPlot::PlotLine( "Expected Change", (double*) nullptr, 0, 1, 0ImPlotItemFlags_NoFit );

                ImPlot::EndPlot( );
            }
        }
    }
}

CombinationTweaker::CombinationTweaker( Loca& LanguageProvider, CombinationMetaCache m_TweakerTarget )
    : LanguageObserver( LanguageProvider )
    , m_EchoNames( LanguageProvider )
    , m_SetNames( LanguageProvider )
    , m_SubStatLabel( LanguageProvider )
    , m_MainStatLabel( LanguageProvider )
    , m_TweakerTarget( std::move( m_TweakerTarget ) )
{
    CombinationTweaker::OnLanguageChanged( &LanguageProvider );

    m_SetNames.SetKeyStrings( { "eFreezingFrost",
                                "eMoltenRift",
                                "eVoidThunder",
                                "eSierraGale",
                                "eCelestialLight",
                                "eSunSinkingEclipse",
                                "eRejuvenatingGlow",
                                "eMoonlitClouds",
                                "eLingeringTunes" } );

    m_SubStatPtr = {
        nullptr,
        &EffectiveStats::flat_attack,
        &EffectiveStats::percentage_attack,
        &EffectiveStats::crit_rate,
        &EffectiveStats::crit_damage,
        &EffectiveStats::auto_attack_buff,
        &EffectiveStats::heavy_attack_buff,
        &EffectiveStats::skill_buff,
        &EffectiveStats::ult_buff,
    };

    m_SubStatLabel.SetKeyStrings( m_SubStatPtr | std::views::transform( EffectiveStats::GetStatName ) );

    InitializeSubStatRollConfigs( );
    InitializePascalTriangle( );
}

void
CombinationTweaker::ApplyStats(
    std::vector<std::pair<FloatTy, ValueRollRate::RateTy>>& Results,
    const SubStatRollConfig**                               PickedRollPtr,
    const EffectiveStats&                                   Stats,
    ValueRollRate::RateTy                                   CurrentRate )
{
    if ( *PickedRollPtr == nullptr )
    {
        Results.emplace_back( m_TweakerTarget.GetEDReplaceEchoAt( m_EchoSlotIndex, Stats ), CurrentRate );
        return;
    }

    const auto& PickedRoll = **PickedRollPtr;

    for ( int i = 0; i < PickedRoll.PossibleRolls; ++i )
    {
        ApplyStats(
            Results,
            PickedRollPtr + 1,
            Stats + StatValueConfig { PickedRoll.ValuePtr, PickedRoll.Values[ i ].Value },
            CurrentRate * PickedRoll.Values[ i ].Rate );
    }
}

void
CombinationTweaker::CalculateFullPotential( )
{
    Stopwatch SP;
    ClearPotentialCache( );

    std::vector<std::pair<FloatTy, ValueRollRate::RateTy>> AllPossibleEcho;
    const auto                                             TweakingCost = m_TweakerTarget.GetEffectiveEchoAtSlot( m_EchoSlotIndex ).Cost;
    EffectiveStats                                         ConfiguredEcho {
                                                .Set    = (EchoSet) m_SelectedEchoSet,
                                                .NameID = m_SelectedEchoNameID,
                                                .Cost   = TweakingCost };

    {
        const StatValueConfig& FirstMainStat  = MaxFirstMainStat[ TweakingCost ][ m_SelectedMainStatTypeIndex ];
        const StatValueConfig& SecondMainStat = MaxSecondMainStat[ TweakingCost ];
        ConfiguredEcho.*( MaxFirstMainStat[ TweakingCost ][ m_SelectedMainStatTypeIndex ].ValuePtr ) += FirstMainStat.Value;
        ConfiguredEcho.*( MaxSecondMainStat[ TweakingCost ].ValuePtr ) += SecondMainStat.Value;
    }

    std::vector<const SubStatRollConfig*> PickedRoll( MaxRollCount + /*  For terminating recursive call */ 1, nullptr );

    for ( int EffectiveRolls = 0; EffectiveRolls <= MaxRollCount; ++EffectiveRolls )
    {
        const auto Duplicates = m_PascalTriangle[ m_NonEffectiveSubStatCount ][ MaxRollCount - EffectiveRolls ];

        std::string bitmask( EffectiveRolls, 1 );
        bitmask.resize( m_SubStatRollConfigs.size( ), 0 );

        std::ranges::fill( PickedRoll, nullptr );
        do
        {
            uint32_t SubStatPickID = 0;
            for ( int j = 0, poll_index = 0; j < m_SubStatRollConfigs.size( ); ++j )
            {
                if ( bitmask[ j ] )
                {
                    SubStatPickID <<= 6;
                    SubStatPickID |= j;
                    PickedRoll[ poll_index++ ] = &m_SubStatRollConfigs[ j ];
                }
            }

            ApplyStats(
                AllPossibleEcho,
                PickedRoll.data( ),
                ConfiguredEcho,
                1.0 * Duplicates );
        } while ( std::ranges::prev_permutation( bitmask ).found );
    }

    m_FullPotentialCache = std::make_unique<EchoPotential>( AnalyzeEchoPotential( AllPossibleEcho ) );
}

void
CombinationTweaker::CalculateEchoPotential( EchoPotential& Result, EffectiveStats CurrentSubStats, int RollRemaining )
{
    Stopwatch SP;
    if ( !m_FullPotentialCache ) CalculateFullPotential( );

    std::vector<std::pair<FloatTy, ValueRollRate::RateTy>> AllPossibleEcho;

    const auto PossibleRolls =
        m_SubStatRollConfigs
        | std::views::filter( [ & ]( const SubStatRollConfig& Config ) {
              return std::abs( CurrentSubStats.*( Config.ValuePtr ) ) < 0.001;
          } )
        | std::ranges::to<std::vector>( );

    if ( PossibleRolls.size( ) < RollRemaining )
    {
        throw std::runtime_error( "Not enough rolls remaining to complete the combination." );
    }

    {
        const auto             TweakingCost   = m_TweakerTarget.GetEffectiveEchoAtSlot( m_EchoSlotIndex ).Cost;
        const StatValueConfig& FirstMainStat  = MaxFirstMainStat[ TweakingCost ][ m_SelectedMainStatTypeIndex ];
        const StatValueConfig& SecondMainStat = MaxSecondMainStat[ TweakingCost ];
        CurrentSubStats.*( MaxFirstMainStat[ TweakingCost ][ m_SelectedMainStatTypeIndex ].ValuePtr ) += FirstMainStat.Value;
        CurrentSubStats.*( MaxSecondMainStat[ TweakingCost ].ValuePtr ) += SecondMainStat.Value;
    }

    std::vector<const SubStatRollConfig*> PickedRoll( RollRemaining + /*  For terminating recursive call */ 1, nullptr );

    const auto UsedNonEffectiveRollCount = ( MaxRollCount - RollRemaining ) - ( m_SubStatRollConfigs.size( ) - PossibleRolls.size( ) );
    if ( UsedNonEffectiveRollCount > m_NonEffectiveSubStatCount ) throw std::runtime_error( "Too many non-effective rolls" );
    for ( int EffectiveRolls = 0; EffectiveRolls <= RollRemaining; ++EffectiveRolls )
    {
        const auto Duplicates = m_PascalTriangle[ m_NonEffectiveSubStatCount - UsedNonEffectiveRollCount ][ RollRemaining - EffectiveRolls ];

        std::string bitmask( EffectiveRolls, 1 );
        bitmask.resize( PossibleRolls.size( ), 0 );

        std::ranges::fill( PickedRoll, nullptr );
        do
        {
            for ( int j = 0, poll_index = 0; j < PossibleRolls.size( ); ++j )
            {
                if ( bitmask[ j ] )
                {
                    PickedRoll[ poll_index++ ] = &PossibleRolls[ j ];
                }
            }

            ApplyStats( AllPossibleEcho, PickedRoll.data( ), CurrentSubStats, 1.0 * Duplicates );

        } while ( std::ranges::prev_permutation( bitmask ).found );
    }

    Result = AnalyzeEchoPotential( AllPossibleEcho );
}

void
CombinationTweaker::InitializeSubStatRollConfigs( )
{

    SubStatRollConfig TemplateConfig;

    m_NonEffectiveSubStatCount = 5;
    // for ( int i = 0; i < 5 /* Non-effective rolls */; ++i )
    //     SubStatRollConfigs.push_back( TemplateConfig );

    TemplateConfig.IsEffective   = true;
    TemplateConfig.PossibleRolls = 8;
    TemplateConfig.Values =
        {
            ValueRollRate {0.064, 0.0739},
            ValueRollRate {0.071,  0.069},
            ValueRollRate {0.079, 0.2072},
            ValueRollRate {0.086,  0.249},
            ValueRollRate {0.094, 0.1823},
            ValueRollRate {0.101,  0.136},
            ValueRollRate {0.109, 0.0534},
            ValueRollRate {0.116, 0.0293}
    };
    TemplateConfig.SetValueStrings( );

    TemplateConfig.ValuePtr = &EffectiveStats::percentage_attack;
    m_SubStatRollConfigs.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::ult_buff;
    m_SubStatRollConfigs.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::heavy_attack_buff;
    m_SubStatRollConfigs.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::skill_buff;
    m_SubStatRollConfigs.push_back( TemplateConfig );
    TemplateConfig.ValuePtr = &EffectiveStats::auto_attack_buff;
    m_SubStatRollConfigs.push_back( TemplateConfig );

    TemplateConfig.ValuePtr = &EffectiveStats::crit_rate;
    std::ranges::for_each( std::views::zip(
                               std::vector<FloatTy> { 0.063, 0.069, 0.075, 0.081, 0.087, 0.093, 0.099, 0.105 },
                               TemplateConfig.Values ),
                           []( std::pair<FloatTy, ValueRollRate&> Pair ) {
                               Pair.second.Value = Pair.first;
                           } );
    TemplateConfig.SetValueStrings( );
    m_SubStatRollConfigs.push_back( TemplateConfig );

    TemplateConfig.ValuePtr = &EffectiveStats::crit_damage;
    std::ranges::for_each( std::views::zip(
                               std::vector<FloatTy> { 0.126, 0.138, 0.15, 0.162, 0.174, 0.186, 0.198, 0.21 },
                               TemplateConfig.Values ),
                           []( std::pair<FloatTy, ValueRollRate&> Pair ) {
                               Pair.second.Value = Pair.first;
                           } );
    TemplateConfig.SetValueStrings( );
    m_SubStatRollConfigs.push_back( TemplateConfig );

    memset( &TemplateConfig.Values, 0, sizeof( TemplateConfig.Values ) );
    TemplateConfig.ValuePtr      = &EffectiveStats::flat_attack;
    TemplateConfig.PossibleRolls = 4;
    TemplateConfig.Values =
        {
            ValueRollRate {30, 0.1243},
            ValueRollRate {40, 0.4621},
            ValueRollRate {50, 0.3857},
            ValueRollRate {60, 0.0279}
    };
    TemplateConfig.SetValueStrings( false );
    m_SubStatRollConfigs.push_back( TemplateConfig );
}

void
CombinationTweaker::InitializePascalTriangle( )
{
    // initialize the first row
    m_PascalTriangle[ 0 ][ 0 ] = 1;   // C(0, 0) = 1

    for ( int i = 1; i < m_PascalTriangle.size( ); i++ )
    {
        m_PascalTriangle[ i ][ 0 ] = 1;   // C(i, 0) = 1
        for ( int j = 1; j <= i; j++ )
        {
            m_PascalTriangle[ i ][ j ] = m_PascalTriangle[ i - 1 ][ j - 1 ] + m_PascalTriangle[ i - 1 ][ j ];
        }
    }
}

EchoPotential
CombinationTweaker::AnalyzeEchoPotential( std::vector<std::pair<FloatTy, ValueRollRate::RateTy>>& DamageDistribution )
{
    EchoPotential Result;
    std::ranges::sort( DamageDistribution );

    Result.BaselineExpectedDamage = m_TweakerTarget.GetExpectedDamage( );
    Result.HighestExpectedDamage  = DamageDistribution.back( ).first;
    Result.LowestExpectedDamage   = DamageDistribution.front( ).first;

    Result.CDF.resize( EchoPotential::CDFResolution );
    Result.CDFSmallOrEqED.resize( EchoPotential::CDFResolution );

    const auto DamageIncreasePerTick = ( Result.HighestExpectedDamage - Result.LowestExpectedDamage ) / EchoPotential::CDFResolution;

    ValueRollRate::RateTy CumulatedRate = 0;
    Result.ExpectedChangeToED           = 0;
    auto       It                       = DamageDistribution.data( );
    const auto ItEnd                    = &DamageDistribution.back( ) + 1;
    for ( int i = 0; i < EchoPotential::CDFResolution; ++i )
    {
        const auto TickDamage = Result.LowestExpectedDamage + i * DamageIncreasePerTick;
        while ( It->first <= TickDamage && It != ItEnd )
        {
            Result.ExpectedChangeToED += It->first * It->second;
            CumulatedRate += It->second;
            It++;
        }

        Result.CDFSmallOrEqED[ i ] = TickDamage;
        Result.CDF[ i ]            = CumulatedRate;
    }

    while ( It != ItEnd )
    {
        Result.ExpectedChangeToED += It->first * It->second;
        CumulatedRate += It->second;
        It++;
    }
    Result.CDFSmallOrEqED.push_back( DamageDistribution.back( ).first );
    Result.CDF.push_back( CumulatedRate );

    Result.CDFChangeToED = Result.CDFSmallOrEqED;
    for ( auto& ED : Result.CDFChangeToED )
        ED = 100 * ED / Result.BaselineExpectedDamage - 100;

    Result.ExpectedChangeToED /= Result.CDF.back( ) * Result.BaselineExpectedDamage / 100;
    Result.ExpectedChangeToED -= 100;

    Result.CDFFloat.resize( Result.CDF.size( ) );
    for ( int i = 0; i < Result.CDF.size( ); ++i )
    {
        Result.CDFFloat[ i ] = Result.CDF[ i ] = ( 1 - Result.CDF[ i ] / Result.CDF.back( ) ) * 100;
    }

    Result.CDFFloat.front( ) = Result.CDF.front( ) = 100;

    return Result;
}
