//
// Created by EMCJava on 7/26/2024.
//

#include "CombinationTweaker.hpp"

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
CombinationTweaker::TweakerMenu( CombinationMetaCache&                                  Target,
                                 const std::map<std::string, std::vector<std::string>>& EchoNamesBySet )
{
    if ( !Target.IsValid( ) ) return;

    ImGui::Dummy( ImVec2 { 0, 5 } );
    if ( ImGui::BeginTable( "SlotEffectiveness", Target.GetSlotCount( ), ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp ) )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_SelectableTextAlign, ImVec2( 0.5f, 0.5f ) );
        ImGui::TableNextRow( );
        for ( int i = 0; i < Target.GetSlotCount( ); ++i )
        {
            ImGui::TableSetColumnIndex( i );
            const auto EDDrop = Target.GetEdDropPercentageWithoutAt( i );
            if ( EDDrop != 0 )
                ImGui::TableSetBgColor( ImGuiTableBgTarget_CellBg, ImGui::GetColorU32( ImVec4( 0.2f + EDDrop * 0.6f, 0.2f, 0.2f, 0.65f ) ) );

            const auto* EchoName = LanguageProvider[ Target.GetEchoNameAtSlot( i ) ];

            ImGui::PushID( i );
            if ( ImGui::Selectable( EchoName, m_EchoSlotIndex == i ) )
                m_EchoSlotIndex = m_EchoSlotIndex != i ? i : -1;
            ImGui::PopID( );
        }
        ImGui::PopStyleVar( );
        ImGui::EndTable( );
    }

    if ( m_EchoSlotIndex != -1 && m_EchoSlotIndex < Target.GetSlotCount( ) )
    {
        auto CurrentTweakingCost = Target.GetEffectiveEchoAtSlot( m_EchoSlotIndex ).Cost;
        if ( PreviousTweakingCost != CurrentTweakingCost )
        {
            CurrentTweakingCost = CurrentTweakingCost;
            m_MainStatLabel.SetKeyStrings(
                MaxFirstMainStat[ CurrentTweakingCost ]
                | std::views::transform( []( auto& Config ) {
                      return EffectiveStats::GetStatName( Config.ValuePtr );
                  } ) );
        }

        const auto& Style = ImGui::GetStyle( );
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
                    EchoNamesBySet.at( SetToString( (EchoSet) m_SelectedEchoSet ) )
                    | std::views::transform( []( auto& Str ) {
                          return Str.c_str( );
                      } )
                    | std::ranges::to<std::vector>( ) );
            }
            ImGui::SameLine( );

            ImGui::SetNextItemWidth( 160 );
            ImGui::Combo( LanguageProvider[ "EchoName" ],
                          &m_SelectedEchoNameID,
                          m_EchoNames.GetRawStrings( ),
                          m_EchoNames.GetStringCount( ) );

            ImGui::SameLine( );
            ImGui::SetNextItemWidth( 120 );
            ImGui::Combo( LanguageProvider[ "MainStat" ],
                          &m_SelectedMainStatTypeIndex,
                          m_MainStatLabel.GetRawStrings( ),
                          m_MainStatLabel.GetStringCount( ) );
        }

        ImGui::SeparatorText( LanguageProvider[ "SubStats" ] );
        {
            const auto EchoConfigWidth = ImGui::GetWindowWidth( );
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
                 && m_SelectedEchoSet >= 0
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

                CalculateEchoPotential( m_SelectedEchoPotential, m_SubStatRollConfigs,
                                        Target, ConfiguredSubStats,
                                        MaxFirstMainStat[ CurrentTweakingCost ][ m_SelectedMainStatTypeIndex ],
                                        MaxSecondMainStat[ CurrentTweakingCost ],
                                        MaxRollCount - m_UsedSubStatCount, m_EchoSlotIndex );
            } else
            {
                m_SelectedEchoPotential.CDF.clear( );
            }
        }
    }

    if ( !m_SelectedEchoPotential.CDF.empty( ) )
    {
        ImGui::SeparatorText( LanguageProvider[ "PotentialEDDis" ] );
        if ( ImGui::BeginTable( "PotentialStats", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame ) )
        {
            ImGui::TableSetupColumn( LanguageProvider[ "BestED" ] );
            ImGui::TableSetupColumn( LanguageProvider[ "WorstED" ] );
            ImGui::TableSetupColumn( LanguageProvider[ "Best%+" ] );
            ImGui::TableSetupColumn( LanguageProvider[ "Worst%-" ] );
            ImGui::TableHeadersRow( );
            ImGui::TableNextRow( );

            ImGui::TableNextColumn( );
            ImGui::Text( "%.2f", m_SelectedEchoPotential.HighestExpectedDamage );
            ImGui::TableNextColumn( );
            ImGui::Text( "%.2f", m_SelectedEchoPotential.LowestExpectedDamage );
            ImGui::TableNextColumn( );
            ImGui::Text( "%.2f%%", m_SelectedEchoPotential.CDFChangeToED.back( ) );
            ImGui::TableNextColumn( );
            ImGui::Text( "%.2f%%", m_SelectedEchoPotential.CDFChangeToED.front( ) );

            ImGui::EndTable( );
        }


        if ( ImPlot::BeginPlot( LanguageProvider[ "Potential" ], ImVec2( -1, 0 ), ImPlotFlags_NoLegend ) )
        {
            ImPlot::SetupLegend( ImPlotLocation_South, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside );
            ImPlot::SetupAxes( LanguageProvider[ "Probability" ], LanguageProvider[ "EDDelta%" ], ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_Invert, ImPlotAxisFlags_AutoFit );

            ImPlot::PlotLine( LanguageProvider[ "OptimalValue" ],
                              m_SelectedEchoPotential.CDFFloat.data( ),
                              m_SelectedEchoPotential.CDFChangeToED.data( ),
                              m_SelectedEchoPotential.CDF.size( ) );

            ImPlot::DragLineY( 0, &m_DragEDTargetY, ImVec4( 1, 0, 0, 1 ), 1, ImPlotDragToolFlags_Delayed );
            ImPlot::TagY( m_DragEDTargetY, ImVec4( 1, 0, 0, 1 ), true );
            m_DragEDTargetY = std::clamp( m_DragEDTargetY,
                                          (double) m_SelectedEchoPotential.CDFChangeToED.front( ),
                                          (double) m_SelectedEchoPotential.CDFChangeToED.back( ) );

            ImPlot::TagX( m_SelectedEchoPotential.CDF[ std::distance(
                              m_SelectedEchoPotential.CDFChangeToED.begin( ),
                              std::ranges::lower_bound( m_SelectedEchoPotential.CDFChangeToED, (FloatTy) m_DragEDTargetY ) ) ],
                          ImVec4( 0, 1, 0, 1 ), true );

            ImPlot::EndPlot( );
        }
    }
}

CombinationTweaker::CombinationTweaker( Loca& LanguageProvider )
    : LanguageObserver( LanguageProvider )
    , m_EchoNames( LanguageProvider )
    , m_SetNames( LanguageProvider )
    , m_SubStatLabel( LanguageProvider )
    , m_MainStatLabel( LanguageProvider )
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
    CombinationMetaCache&                                   Environment,
    const SubStatRollConfig**                               PickedRollPtr,
    const EffectiveStats&                                   Stats,
    int                                                     SlotIndex,
    ValueRollRate::RateTy                                   CurrentRate )
{
    if ( *PickedRollPtr == nullptr )
    {
        Results.emplace_back( Environment.GetEDReplaceEchoAt( SlotIndex, Stats ), CurrentRate );
        std::ranges::push_heap( Results );
        return;
    }

    const auto& PickedRoll = **PickedRollPtr;

    for ( int i = 0; i < PickedRoll.PossibleRolls; ++i )
    {
        ApplyStats(
            Results,
            Environment,
            PickedRollPtr + 1,
            Stats + StatValueConfig { PickedRoll.ValuePtr, PickedRoll.Values[ i ].Value },
            SlotIndex,
            CurrentRate * PickedRoll.Values[ i ].Rate );
    }
}

void
CombinationTweaker::CalculateEchoPotential( EchoPotential& Result, const std::vector<SubStatRollConfig>& RollConfigs, CombinationMetaCache& Environment, EffectiveStats CurrentSubStats, const StatValueConfig& FirstMainStat, const StatValueConfig& SecondMainStat, int RollRemaining, int SlotIndex )
{
    Stopwatch SP;

    std::vector<std::pair<FloatTy, ValueRollRate::RateTy>> AllPossibleEcho;

    const auto PossibleRolls =
        RollConfigs
        | std::views::filter( [ & ]( const SubStatRollConfig& Config ) {
              return std::abs( CurrentSubStats.*( Config.ValuePtr ) ) < 0.001;
          } )
        | std::ranges::to<std::vector>( );

    if ( PossibleRolls.size( ) < RollRemaining )
    {
        throw std::runtime_error( "Not enough rolls remaining to complete the combination." );
    }

    if ( FirstMainStat.ValuePtr != nullptr ) CurrentSubStats.*( FirstMainStat.ValuePtr ) += FirstMainStat.Value;
    if ( SecondMainStat.ValuePtr != nullptr ) CurrentSubStats.*( SecondMainStat.ValuePtr ) += SecondMainStat.Value;

    std::vector<const SubStatRollConfig*> PickedRoll( RollRemaining + /*  For terminating recursive call */ 1, nullptr );

    const auto NonEffectiveRollCount = ( MaxRollCount - RollRemaining ) - ( RollConfigs.size( ) - PossibleRolls.size( ) );
    if ( NonEffectiveRollCount > m_NonEffectiveSubStatCount ) throw std::runtime_error( "Too many non-effective rolls" );
    for ( int EffectiveRolls = 0; EffectiveRolls <= RollRemaining; ++EffectiveRolls )
    {
        const auto Duplicates = m_PascalTriangle[ m_NonEffectiveSubStatCount - NonEffectiveRollCount ][ RollRemaining - EffectiveRolls ];

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

            ApplyStats(
                AllPossibleEcho,
                Environment,
                PickedRoll.data( ),
                CurrentSubStats,
                SlotIndex,
                1.0 * Duplicates );
        } while ( std::prev_permutation( bitmask.begin( ), bitmask.end( ) ) );
    }

    std::ranges::sort_heap( AllPossibleEcho );

    Result.BaselineExpectedDamage = Environment.GetExpectedDamage( );
    Result.HighestExpectedDamage  = AllPossibleEcho.back( ).first;
    Result.LowestExpectedDamage   = AllPossibleEcho.front( ).first;

    Result.CDF.resize( EchoPotential::CDFResolution );
    Result.CDFSmallOrEqED.resize( EchoPotential::CDFResolution );

    const auto DamageIncreasePerTick = ( Result.HighestExpectedDamage - Result.LowestExpectedDamage ) / EchoPotential::CDFResolution;

    ValueRollRate::RateTy CumulatedRate = 0;
    auto                  It            = AllPossibleEcho.data( );
    const auto            ItEnd         = &AllPossibleEcho.back( ) + 1;
    for ( int i = 0; i < EchoPotential::CDFResolution; ++i )
    {
        const auto TickDamage = Result.LowestExpectedDamage + i * DamageIncreasePerTick;
        while ( It->first <= TickDamage && It != ItEnd )
        {
            CumulatedRate += It->second;
            It++;
        }

        Result.CDFSmallOrEqED[ i ] = TickDamage;
        Result.CDF[ i ]            = CumulatedRate;
    }

    while ( It != ItEnd )
    {
        CumulatedRate += It->second;
        It++;
    }
    Result.CDFSmallOrEqED.push_back( AllPossibleEcho.back( ).first );
    Result.CDF.push_back( CumulatedRate );

    Result.CDFChangeToED = Result.CDFSmallOrEqED;
    for ( auto& ED : Result.CDFChangeToED )
        ED = 100 * ED / Result.BaselineExpectedDamage - 100;

    Result.CDFFloat.resize( Result.CDF.size( ) );
    for ( int i = 0; i < Result.CDF.size( ); ++i )
    {
        Result.CDFFloat[ i ] = Result.CDF[ i ] = 1 - Result.CDF[ i ] / Result.CDF.back( );
    }

    Result.CDFFloat.front( ) = Result.CDF.front( ) = 1;
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
