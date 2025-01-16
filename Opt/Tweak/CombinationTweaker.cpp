//
// Created by EMCJava on 7/26/2024.
//

#include "CombinationTweaker.hpp"

#include <Opt/UI/UIConfig.hpp>

#include <Opt/OptUtil.hpp>

#include <magic_enum/magic_enum.hpp>

#include <imgui.h>
#include <imgui-SFML.h>

#include <format>

inline const char* CostNames[] = { "0", "1", "2", "3", "4" };

inline std::array<std::vector<StatValueConfig>, 5 /* Indexed by cost */> MaxFirstMainStat {
    std::vector<StatValueConfig> { },
    std::vector<StatValueConfig> {
                                  StatValueConfig { nullptr, 0 },
                                  StatValueConfig { &EffectiveStats::percentage_attack, 0.18 },
                                  StatValueConfig { &EffectiveStats::percentage_health, 0.228 },
                                  StatValueConfig { &EffectiveStats::percentage_defence, 0.18 } },
    std::vector<StatValueConfig> { },
    std::vector<StatValueConfig> {
                                  StatValueConfig { nullptr, 0 },
                                  StatValueConfig { &EffectiveStats::percentage_attack, 0.3 },
                                  StatValueConfig { &EffectiveStats::percentage_health, 0.3 },
                                  StatValueConfig { &EffectiveStats::percentage_defence, 0.38 },
                                  StatValueConfig { &EffectiveStats::buff_multiplier, 0.3 } },
    std::vector<StatValueConfig> {
                                  StatValueConfig { nullptr, 0 },
                                  StatValueConfig { &EffectiveStats::heal_buff, 0.264 },
                                  StatValueConfig { &EffectiveStats::percentage_attack, 0.33 },
                                  StatValueConfig { &EffectiveStats::percentage_health, 0.33 },
                                  StatValueConfig { &EffectiveStats::percentage_defence, 0.418 },
                                  StatValueConfig { &EffectiveStats::crit_rate, 0.22 },
                                  StatValueConfig { &EffectiveStats::crit_damage, 0.44 },
                                  }
};

inline std::array<StatValueConfig, 5 /* Indexed by cost */> MaxSecondMainStat {
    StatValueConfig { },
    StatValueConfig { &EffectiveStats::flat_health, 2280 },
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

std::array<std::array<long long, 14>, 14> CombinationTweaker::PascalTriangle = []( ) {
    std::array<std::array<long long, 14>, 14> Result { };

    // initialize the first row
    Result[ 0 ][ 0 ] = 1;   // C(0, 0) = 1

    for ( int i = 1; i < Result.size( ); i++ )
    {
        Result[ i ][ 0 ] = 1;   // C(i, 0) = 1
        for ( int j = 1; j <= i; j++ )
        {
            Result[ i ][ j ] = Result[ i - 1 ][ j - 1 ] + Result[ i - 1 ][ j ];
        }
    }

    return Result;
}( );

CombinationTweaker::CombinationTweaker( const CombinationMetaCache& m_TweakerTarget )
    : m_TweakerTarget( m_TweakerTarget )
{
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
        Results.emplace_back( m_TweakerTarget.GetOVReplaceEchoAt( m_TweakingEchoSlot, Stats ), CurrentRate );
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

std::unique_ptr<EchoPotential>
CombinationTweaker::CalculateFullPotential( int                                   EchoSlot,
                                            EchoSet                               Set,
                                            int                                   EchoNameID,
                                            const StatValueConfig&                PrimaryStat,
                                            const StatValueConfig&                SecondaryStat,
                                            int                                   NonEffectiveRollCount,
                                            const std::vector<SubStatRollConfig>* RollConfigs )
{
    Stopwatch SP;
    ClearPotentialCache( );

    std::vector<std::pair<FloatTy, ValueRollRate::RateTy>> AllPossibleEcho;

    m_TweakingEchoSlot = EchoSlot;

    const auto TweakingCost = m_TweakerTarget.GetEffectiveEchoAtSlot( m_TweakingEchoSlot ).Cost;

    EffectiveStats ConfiguredEcho { .Set = Set, .NameID = EchoNameID, .Cost = TweakingCost };

    if ( PrimaryStat.ValuePtr != nullptr )
        ConfiguredEcho.*( PrimaryStat.ValuePtr ) += PrimaryStat.Value;
    if ( SecondaryStat.ValuePtr != nullptr )
        ConfiguredEcho.*( SecondaryStat.ValuePtr ) += SecondaryStat.Value;

    std::vector<const SubStatRollConfig*> PickedRoll( MaxRollCount + /*  For terminating recursive call */ 1, nullptr );

    for ( int EffectiveRolls = MaxRollCount - NonEffectiveRollCount; EffectiveRolls <= MaxRollCount; ++EffectiveRolls )
    {
        const auto Duplicates = PascalTriangle[ NonEffectiveRollCount ][ MaxRollCount - EffectiveRolls ];

        std::string bitmask( EffectiveRolls, 1 );
        bitmask.resize( RollConfigs->size( ), 0 );

        std::ranges::fill( PickedRoll, nullptr );
        do
        {
            for ( int j = 0, poll_index = 0; j < RollConfigs->size( ); ++j )
            {
                if ( bitmask[ j ] )
                {
                    PickedRoll[ poll_index++ ] = &RollConfigs->at( j );
                }
            }

            ApplyStats(
                AllPossibleEcho,
                PickedRoll.data( ),
                ConfiguredEcho,
                1.0 * Duplicates );
        } while ( std::ranges::prev_permutation( bitmask ).found );
    }

    return std::make_unique<EchoPotential>( AnalyzeEchoPotential( AllPossibleEcho ) );
}

void
CombinationTweaker::CalculateEchoPotential( EchoPotential& Result, int EchoSlot, EffectiveStats CurrentSubStats, const StatValueConfig& PrimaryStat, const StatValueConfig& SecondaryStat, int RollRemaining )
{
    Stopwatch SP;
    if ( !m_FullPotentialCache )
    {
        m_FullPotentialCache = CalculateFullPotential( EchoSlot,
                                                       CurrentSubStats.Set,
                                                       CurrentSubStats.NameID,
                                                       PrimaryStat,
                                                       SecondaryStat );
    }

    m_TweakingEchoSlot = EchoSlot;
    std::vector<std::pair<FloatTy, ValueRollRate::RateTy>> AllPossibleEcho;

    const auto PossibleRolls =
        FullSubStatRollConfigs
        | std::views::filter( [ & ]( const SubStatRollConfig& Config ) {
              return std::abs( CurrentSubStats.*( Config.ValuePtr ) ) < 0.001;
          } )
        | std::ranges::to<std::vector>( );

    if ( PossibleRolls.size( ) < RollRemaining )
    {
        throw std::runtime_error( "Not enough rolls remaining to complete the combination." );
    }

    if ( PrimaryStat.ValuePtr != nullptr )
        CurrentSubStats.*( PrimaryStat.ValuePtr ) += PrimaryStat.Value;
    if ( SecondaryStat.ValuePtr != nullptr )
        CurrentSubStats.*( SecondaryStat.ValuePtr ) += SecondaryStat.Value;

    std::vector<const SubStatRollConfig*> PickedRoll( RollRemaining + /*  For terminating recursive call */ 1, nullptr );

    const auto UsedNonEffectiveRollCount = ( MaxRollCount - RollRemaining ) - ( FullSubStatRollConfigs.size( ) - PossibleRolls.size( ) );
    if ( UsedNonEffectiveRollCount > MaxNonEffectiveSubStatCount ) throw std::runtime_error( "Too many non-effective rolls" );
    for ( int EffectiveRolls = 0; EffectiveRolls <= RollRemaining; ++EffectiveRolls )
    {
        const auto Duplicates = PascalTriangle[ MaxNonEffectiveSubStatCount - UsedNonEffectiveRollCount ][ RollRemaining - EffectiveRolls ];

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

EchoPotential
CombinationTweaker::AnalyzeEchoPotential( std::vector<std::pair<FloatTy, ValueRollRate::RateTy>>& OptimizingValueDistribution )
{
    EchoPotential Result;
    std::ranges::sort( OptimizingValueDistribution );

    Result.BaselineOptimizingValue = m_TweakerTarget.GetOptimizingValue( );
    Result.HighestOptimizingValue  = OptimizingValueDistribution.back( ).first;
    Result.LowestOptimizingValue   = OptimizingValueDistribution.front( ).first;

    Result.CDF.resize( EchoPotential::CDFResolution );
    Result.CDFSmallOrEqOV.resize( EchoPotential::CDFResolution );

    const auto OptimizingValueIncreasePerTick = ( Result.HighestOptimizingValue - Result.LowestOptimizingValue ) / EchoPotential::CDFResolution;

    ValueRollRate::RateTy CumulatedRate = 0;
    Result.ExpectedChangeToOV           = 0;
    auto       It                       = OptimizingValueDistribution.data( );
    const auto ItEnd                    = &OptimizingValueDistribution.back( ) + 1;
    for ( int i = 0; i < EchoPotential::CDFResolution; ++i )
    {
        const auto TickValue = Result.LowestOptimizingValue + i * OptimizingValueIncreasePerTick;
        while ( It->first <= TickValue && It != ItEnd )
        {
            Result.ExpectedChangeToOV += It->first * It->second;
            CumulatedRate += It->second;
            It++;
        }

        Result.CDFSmallOrEqOV[ i ] = TickValue;
        Result.CDF[ i ]            = CumulatedRate;
    }

    while ( It != ItEnd )
    {
        Result.ExpectedChangeToOV += It->first * It->second;
        CumulatedRate += It->second;
        It++;
    }
    Result.CDFSmallOrEqOV.push_back( OptimizingValueDistribution.back( ).first );
    Result.CDF.push_back( CumulatedRate );

    Result.CDFChangeToOV = Result.CDFSmallOrEqOV;
    for ( auto& ED : Result.CDFChangeToOV )
        ED = 100 * ED / Result.BaselineOptimizingValue - 100;

    Result.ExpectedChangeToOV /= Result.CDF.back( ) * Result.BaselineOptimizingValue / 100;
    Result.ExpectedChangeToOV -= 100;

    Result.CDFFloat.resize( Result.CDF.size( ) );
    for ( int i = 0; i < Result.CDF.size( ); ++i )
    {
        Result.CDFFloat[ i ] = Result.CDF[ i ] = ( 1 - Result.CDF[ i ] / Result.CDF.back( ) ) * 100;
    }

    Result.CDFFloat.front( ) = Result.CDF.front( ) = 100;

    return Result;
}

std::pair<FloatTy, FloatTy>
CombinationTweaker::CalculateSubStatMinMaxExpectedDamage( int EchoSlot )
{
    int     MaxMainStat = 1;
    FloatTy MaxResult   = std::numeric_limits<FloatTy>::lowest( );

    const auto& OldStats      = m_TweakerTarget.GetEffectiveEchoAtSlot( EchoSlot );
    const auto& PrimaryStats  = MaxFirstMainStat[ OldStats.Cost ];
    const auto& SecondaryStat = MaxSecondMainStat[ OldStats.Cost ];

    for ( int i = /* Ignore non-effective main stat */ 1; i < PrimaryStats.size( ); ++i )
    {
        auto Potential =
            CalculateFullPotential( EchoSlot, OldStats.Set, OldStats.NameID,
                                    PrimaryStats[ i ], SecondaryStat,
                                    0, &MaxSubStatRollConfigs );

        if ( Potential->HighestOptimizingValue > MaxResult )
        {
            MaxResult   = Potential->HighestOptimizingValue;
            MaxMainStat = i;
        }
    }

    EffectiveStats LowestEcho { .Set = OldStats.Set, .NameID = OldStats.NameID, .Cost = OldStats.Cost };
    LowestEcho.*( PrimaryStats[ MaxMainStat ].ValuePtr ) += PrimaryStats[ MaxMainStat ].Value;
    LowestEcho.*( SecondaryStat.ValuePtr ) += SecondaryStat.Value;

    return std::make_pair( m_TweakerTarget.GetOVReplaceEchoAt( EchoSlot, LowestEcho ), MaxResult );
}

CombinationTweakerMenu::CombinationTweakerMenu( Loca& LanguageProvider, const CombinationMetaCache& Target )
    : LanguageObserver( LanguageProvider )
    , CombinationTweaker( Target )
    , m_EchoNames( LanguageProvider )
    , m_SetNames( LanguageProvider )
    , m_SubStatLabel( LanguageProvider )
    , m_MainStatLabel( LanguageProvider )
{
    CombinationTweakerMenu::OnLanguageChanged( &LanguageProvider );

    m_SetNames.SetKeyStrings( {
        "eFreezingFrost",
        "eMoltenRift",
        "eVoidThunder",
        "eSierraGale",
        "eCelestialLight",
        "eSunSinkingEclipse",
        "eRejuvenatingGlow",
        "eMoonlitClouds",
        "eLingeringTunes",
        "eFrostyResolve",
        "eEternalRadiance",
        "eMidnightVeil",
        "eEmpyreanAnthem",
        "eTidebreakingCourage",
    } );

    m_SubStatPtr = {
        nullptr,
        &EffectiveStats::flat_health,
        &EffectiveStats::flat_attack,
        &EffectiveStats::flat_defence,
        &EffectiveStats::percentage_health,
        &EffectiveStats::percentage_attack,
        &EffectiveStats::percentage_defence,
        &EffectiveStats::crit_rate,
        &EffectiveStats::crit_damage,
        &EffectiveStats::auto_attack_buff,
        &EffectiveStats::heavy_attack_buff,
        &EffectiveStats::skill_buff,
        &EffectiveStats::ult_buff,
    };

    m_SubStatLabel.SetKeyStrings( m_SubStatPtr | std::views::transform( EffectiveStats::GetStatName ) );
}

void
CombinationTweakerMenu::TweakerMenu( const std::map<std::string, std::vector<std::string>>& EchoNamesBySet )
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
            const auto EchoScore = m_TweakerTarget.GetEchoSigmoidScoreAt( i );
            ImGui::TableSetBgColor( ImGuiTableBgTarget_CellBg, ImGui::GetColorU32( ImVec4( EchoScore < 0.5 ? 1 : 1 - ( EchoScore * 2 - 1 ) * 0.9f - 0.1f, EchoScore > 0.5 ? 1 : ( EchoScore * 2 ) * 0.9f + 0.1f, 0, 0.45f ) ) );

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
                auto Names =
                    EchoNamesBySet.at( std::string( magic_enum::enum_name( static_cast<EchoSet>( m_SelectedEchoSet ) ) ) )
                    | std::views::transform( []( auto& Str ) {
                          return Str.c_str( );
                      } )
                    | std::ranges::to<std::vector>( );
                Names.insert( Names.begin( ), "Versatile" );
                m_EchoNames.SetKeyStrings( Names );
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

                const auto* SubStatLabelRwaString    = m_SubStatLabel.GetRawStrings( );
                auto&       SelectedSubStatTypeIndex = m_SelectedSubStatTypeIndices[ i ];

                if ( ImGui::BeginCombo( LanguageProvider[ "SubStatType" ],
                                        SubStatLabelRwaString[ SelectedSubStatTypeIndex ] ) )   // The second parameter is the label previewed before opening the combo.
                {
                    for ( int n = 0; n < m_SubStatLabel.GetStringCount( ); n++ )
                    {
                        bool       IsSelected    = n == SelectedSubStatTypeIndex;
                        const bool ShouldDisable = !IsSelected && m_SelectedSubStatTypeSet.contains( n );

                        if ( ShouldDisable ) ImGui::BeginDisabled( );
                        if ( ImGui::Selectable( SubStatLabelRwaString[ n ], IsSelected ) )
                        {
                            m_ShouldRecalculateSubStatConfigs = true;
                            m_SelectedSubStatTypeSet.erase( SelectedSubStatTypeIndex );
                            m_SelectedSubStatTypeSet.insert( SelectedSubStatTypeIndex = n );
                        }
                        if ( ShouldDisable ) ImGui::EndDisabled( );

                        if ( IsSelected )
                            ImGui::SetItemDefaultFocus( );   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                    }
                    ImGui::EndCombo( );
                }

                ImGui::SameLine( );

                const auto SelectedTypeIt =
                    std::ranges::find_if( FullSubStatRollConfigs | std::views::reverse,
                                          [ Ptr = m_SubStatPtr[ m_SelectedSubStatTypeIndices[ i ] ] ]( const SubStatRollConfig& Config ) {
                                              return Config.ValuePtr == Ptr;
                                          } );
                if ( SelectedTypeIt == FullSubStatRollConfigs.rend( ) )
                {
                    ImGui::BeginDisabled( );
                    ImGui::Combo( LanguageProvider[ "SubStatValue" ],
                                  &m_SelectedSubStatValueIndices[ i ],
                                  SubStatRollConfig::GetValueString,
                                  nullptr, 0 );
                    ImGui::EndDisabled( );
                } else
                {
                    m_ShouldRecalculateSubStatConfigs |= ImGui::Combo( LanguageProvider[ "SubStatValue" ],
                                             &m_SelectedSubStatValueIndices[ i ],
                                             SubStatRollConfig::GetValueString,
                                             (void*) &*SelectedTypeIt,
                                             SelectedTypeIt->PossibleRolls );
                }

                ImGui::EndChild( );
                ImGui::PopID( );

                if ( m_ShouldRecalculateSubStatConfigs )
                {
                    if ( SelectedTypeIt != FullSubStatRollConfigs.rend( ) )
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
            if ( ImGui::Selectable( "+", true, ImGuiSelectableFlags_None, ImVec2( EchoConfigWidth / 2 - Style.WindowPadding.x * 2, 10 ) ) )
            {
                // Prevent repeating sub-stat
                if ( m_SelectedSubStatTypeSet.contains( m_SelectedSubStatTypeIndices[ m_UsedSubStatCount ] ) )
                {
                    for ( int i = 0; i < m_SubStatLabel.GetStringCount( ); ++i )
                    {
                        if ( !m_SelectedSubStatTypeSet.contains( i ) )
                        {
                            m_SelectedSubStatTypeIndices[ m_UsedSubStatCount ] = i;
                            break;
                        }
                    }
                }

                m_SelectedSubStatTypeSet.insert( m_SelectedSubStatTypeIndices[ m_UsedSubStatCount ] );
                m_UsedSubStatCount++;
                m_ShouldRecalculateSubStatConfigs = true;
            }
            if ( CantAdd ) ImGui::EndDisabled( );

            ImGui::SameLine( );

            const bool CantSub = m_UsedSubStatCount <= 0;
            if ( CantSub ) ImGui::BeginDisabled( );
            if ( ImGui::Selectable( "-", true, ImGuiSelectableFlags_None, ImVec2( EchoConfigWidth / 2 - Style.WindowPadding.x * 2, 10 ) ) )
            {
                m_UsedSubStatCount--;
                m_SelectedSubStatTypeSet.erase( m_SelectedSubStatTypeIndices[ m_UsedSubStatCount ] );
                m_ShouldRecalculateSubStatConfigs = true;
            }
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
                    .Set = (EchoSet) m_SelectedEchoSet,
                    // Versatile option
                    .NameID = ( m_SelectedEchoNameID == 0 ? (int) m_EchoNames.GetStringCount( ) : m_SelectedEchoNameID ) - 1,
                    .Cost   = CurrentTweakingCost };

                for ( int i = 0; i < m_UsedSubStatCount; ++i )
                {
                    if ( m_EchoSubStatConfigs[ i ].ValuePtr != nullptr )
                    {
                        ConfiguredSubStats.*( m_EchoSubStatConfigs[ i ].ValuePtr ) += m_EchoSubStatConfigs[ i ].Value;
                    }
                }

                const auto             TweakingCost   = m_TweakerTarget.GetEffectiveEchoAtSlot( m_EchoSlotIndex ).Cost;
                const StatValueConfig& FirstMainStat  = MaxFirstMainStat[ TweakingCost ][ m_SelectedMainStatTypeIndex ];
                const StatValueConfig& SecondMainStat = MaxSecondMainStat[ TweakingCost ];

                CalculateEchoPotential( m_SelectedEchoPotential,
                                        m_EchoSlotIndex,
                                        ConfiguredSubStats,
                                        FirstMainStat, SecondMainStat,
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
            std::clamp( (size_t) std::distance(
                            m_SelectedEchoPotential.CDFChangeToOV.begin( ),
                            std::ranges::lower_bound( m_SelectedEchoPotential.CDFChangeToOV, (FloatTy) m_DragEDTargetY ) ),
                        0ULL, m_SelectedEchoPotential.CDFChangeToOV.size( ) - 1 );

        if ( m_FullPotentialCache )
        {
            const auto SimpleViewWidth = EchoConfigWidth - Style.WindowPadding.x * 2;
            ImGui::BeginChild( "Yes/No", ImVec2( SimpleViewWidth, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY );
            ImGui::Spacing( );

            UIConfig::PushFont( UIConfig::Big );
            static constexpr std::array<int, 6> CumulativeEXP { 0, 4400, 16500, 39600, 79100, 142600 };

            const auto UsedEXP = CumulativeEXP[ m_UsedSubStatCount ];

            const auto RenewableEXP  = static_cast<decltype( UsedEXP )>( UsedEXP * 0.75 );
            const auto NewEchoEXPReq = CumulativeEXP.back( ) - RenewableEXP;

            const auto ImprovementPerEXPNew   = m_FullPotentialCache->ExpectedChangeToOV / NewEchoEXPReq;
            const auto ImprovementPerEXPStick = m_SelectedEchoPotential.ExpectedChangeToOV / ( CumulativeEXP.back( ) - UsedEXP );
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

                    UIConfig::PushFont( UIConfig::Big );

                    if ( OptionalLock.has_value( ) )
                    {
                        bool ShouldResetValue = false;
                        ImGui::SetCursorPosX( ( SimpleViewWidth - NumberFrameWidth * 2 ) / 4 );
                        ImGui::BeginChild( (ImGuiID) &OptionalLock, ImVec2( NumberFrameWidth, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY );
                        ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.f, 0.f, 0.f, 0.f ) );

                        DisplayTextAt( std::format( "{:.3f} %", OptionalLock.value( ) ).c_str( ), 0.5 );
                        const auto LockIcon = UIConfig::GetTextureOrDefault( "Lock" );
                        if ( LockIcon )
                        {
                            ImGui::SetCursorPos( ImVec2 { NumberFrameWidth - IconSize - Style.WindowPadding.x, 0 } );
                            if ( ImGui::ImageButton( "LockImageButton", *LockIcon, sf::Vector2f( IconSize, IconSize ) ) )
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
                        const auto UnlockIcon = UIConfig::GetTextureOrDefault( "Unlock" );
                        if ( UnlockIcon )
                        {
                            ImGui::SetCursorPos( ImVec2 { NumberFrameWidth - IconSize - Style.WindowPadding.x, 0 } );
                            if ( ImGui::ImageButton( "UnlockImageButton", *UnlockIcon, sf::Vector2f( IconSize, IconSize ) ) )
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
            DisplayNumberCompare( m_PinnedExpectedChange, m_SelectedEchoPotential.ExpectedChangeToOV );
            ImGui::NewLine( );

            ImGui::PopStyleColor( 3 );

            ImGui::EndChild( );
        }

        if ( ImGui::CollapsingHeader( LanguageProvider[ "PotentialOVDis" ] ) )
        {
            if ( ImGui::BeginTable( "PotentialStats", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame ) )
            {
                ImGui::TableSetupColumn( LanguageProvider[ "BestOV" ] );
                ImGui::TableSetupColumn( LanguageProvider[ "Baseline" ] );
                ImGui::TableSetupColumn( LanguageProvider[ "WorstOV" ] );
                ImGui::TableHeadersRow( );
                ImGui::TableNextRow( );

                ImGui::TableNextColumn( );
                ImGui::Text( m_SelectedEchoPotential.CDFChangeToOV.back( ) > 0 ? "%.2f     (+%.2f%%)" : "%.2f     (%.2f%%)",
                             m_SelectedEchoPotential.HighestOptimizingValue,
                             m_SelectedEchoPotential.CDFChangeToOV.back( ) );
                ImGui::TableNextColumn( );
                ImGui::Text( "%.2f", m_SelectedEchoPotential.BaselineOptimizingValue );
                ImGui::TableNextColumn( );
                ImGui::Text( m_SelectedEchoPotential.CDFChangeToOV.front( ) > 0 ? "%.2f     (+%.2f%%)" : "%.2f     (%.2f%%)",
                             m_SelectedEchoPotential.LowestOptimizingValue,
                             m_SelectedEchoPotential.CDFChangeToOV.front( ) );

                ImGui::EndTable( );
            }

            if ( ImPlot::BeginPlot( LanguageProvider[ "Potential" ], ImVec2( -1, 0 ) ) )
            {
                ImPlot::SetupLegend( ImPlotLocation_North | ImPlotLocation_West, ImPlotLegendFlags_Outside - 1 );
                ImPlot::SetupAxes( LanguageProvider[ "Probability%" ], LanguageProvider[ "EDDelta%" ], ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_Invert, ImPlotAxisFlags_AutoFit );
                ImPlot::SetupAxis( ImAxis_Y2, LanguageProvider[ "ExpectedDamage" ], ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_AutoFit );
                ImPlot::SetupAxisLimitsConstraints( ImAxis_Y2, m_SelectedEchoPotential.CDFSmallOrEqOV.front( ), m_SelectedEchoPotential.CDFSmallOrEqOV.back( ) );

                ImPlot::SetAxes( ImAxis_X1, ImAxis_Y2 );
                ImPlot::PlotLine( "ED_CDF",
                                  m_SelectedEchoPotential.CDFFloat.data( ),
                                  m_SelectedEchoPotential.CDFSmallOrEqOV.data( ),
                                  m_SelectedEchoPotential.CDF.size( ), ImPlotItemFlags_NoLegend );

                ImPlot::SetAxes( ImAxis_X1, ImAxis_Y1 );
                ImPlot::PlotLine( "Delta%CDF",
                                  m_SelectedEchoPotential.CDFFloat.data( ),
                                  m_SelectedEchoPotential.CDFChangeToOV.data( ),
                                  m_SelectedEchoPotential.CDF.size( ), ImPlotItemFlags_NoLegend );

                if ( m_SelectedEchoPotential.CDF.size( ) > SelectedEDIndex )
                {
                    ImPlot::SetNextFillStyle( ImVec4 { 0, 1, 0, 1 }, 0.2 );
                    ImPlot::PlotShaded( "Delta%CDFS",
                                        m_SelectedEchoPotential.CDFFloat.data( ) + SelectedEDIndex,
                                        m_SelectedEchoPotential.CDFChangeToOV.data( ) + SelectedEDIndex,
                                        m_SelectedEchoPotential.CDF.size( ) - SelectedEDIndex, m_DragEDTargetY, ImPlotItemFlags_NoLegend );
                }

                ImPlot::TagX( m_SelectedEchoPotential.CDF[ SelectedEDIndex ], ImVec4( 0, 1, 0, 1 ), "%.2f%%", m_SelectedEchoPotential.CDF[ SelectedEDIndex ] );

                ImPlot::DragLineY( 1, &m_SelectedEchoPotential.ExpectedChangeToOV, ImVec4( 1, 1, 0, 1 ), 1, ImPlotDragToolFlags_NoInputs | ImPlotDragToolFlags_NoFit );
                ImPlot::TagY( m_SelectedEchoPotential.ExpectedChangeToOV, ImVec4( 1, 1, 0, 1 ), true );

                ImPlot::DragLineY( 0, &m_DragEDTargetY, ImVec4( 1, 0, 0, 1 ), 1, ImPlotDragToolFlags_Delayed );
                ImPlot::TagY( m_DragEDTargetY, ImVec4( 1, 0, 0, 1 ), true );
                m_DragEDTargetY = std::clamp( m_DragEDTargetY,
                                              (double) m_SelectedEchoPotential.CDFChangeToOV.front( ),
                                              (double) m_SelectedEchoPotential.CDFChangeToOV.back( ) );

                ImPlot::SetNextLineStyle( ImVec4( 1, 1, 0, 1 ) );
                ImPlot::PlotDummy( LanguageProvider[ "EEDDelta%" ] );
                // ImPlot::PlotLine( "Expected Change", (double*) nullptr, 0, 1, 0ImPlotItemFlags_NoFit );

                ImPlot::EndPlot( );
            }
        }
    }
}
