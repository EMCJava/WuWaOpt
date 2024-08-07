//
// Created by EMCJava on 8/2/2024.
//

#include "Backpack.hpp"

#include <Opt/UI/OptimizerUIConfig.hpp>

#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_internal.h>

#include <magic_enum.hpp>

#include <spdlog/spdlog.h>

#include <fstream>

inline std::array<sf::Color, (int) EchoSet::eEchoSetCount + 1> EchoSetSFColor {
    sf::Color( 66, 178, 255, 255 ),
    sf::Color( 245, 118, 79, 255 ),
    sf::Color( 182, 108, 255, 255 ),
    sf::Color( 86, 255, 183, 255 ),
    sf::Color( 247, 228, 107, 255 ),
    sf::Color( 204, 141, 181, 255 ),
    sf::Color( 135, 189, 41, 255 ),
    sf::Color( 255, 255, 255, 255 ),
    sf::Color( 202, 44, 37, 255 ),
    sf::Color( 243, 60, 241, 255 ) };

Backpack::Backpack( const std::string& EchoesPath, const std::map<std::string, std::vector<std::string>>& EchoNamesBySet, Loca& LocaObj )
    : LanguageObserver( LocaObj )
    , m_EchoesPath( EchoesPath )
{
    m_SetFilter.fill( true );

    std::ifstream EchoFile { m_EchoesPath };
    if ( !EchoFile )
    {
        spdlog::error( "Failed to open echoes file." );
        system( "pause" );
        exit( 1 );
    }

    auto FullStatsList = YAML::Load( EchoFile ).as<std::vector<FullStats>>( )
        | std::views::filter( [ & ]( FullStats& FullEcho ) {
                             const auto NameListIt = EchoNamesBySet.find( std::string( FullEcho.GetSetName( ) ) );
                             if ( NameListIt == EchoNamesBySet.end( ) ) return false;

                             const auto NameIt = std::ranges::find( NameListIt->second, FullEcho.EchoName );
                             if ( NameIt == NameListIt->second.end( ) ) return false;

                             FullEcho.NameID = std::distance( NameListIt->second.begin( ), NameIt );
                             return true;
                         } )
        | std::ranges::to<std::vector>( );

    if ( FullStatsList.empty( ) )
    {
        spdlog::critical( "No valid echoes found in the provided file." );
        system( "pause" );
        exit( 1 );
    }

    std::ranges::sort( FullStatsList, []( const auto& EchoA, const auto& EchoB ) {
        if ( EchoA.Cost > EchoB.Cost ) return true;
        if ( EchoA.Cost < EchoB.Cost ) return false;
        if ( EchoA.Level > EchoB.Level ) return true;
        if ( EchoA.Level < EchoB.Level ) return false;
        return false;
    } );

    Set( FullStatsList );
}

bool
Backpack::DisplayBackpack( )
{
    bool        Changed  = false;
    const auto& Style    = ImGui::GetStyle( );
    const auto  BPressed = ImGui::IsKeyPressed( ImGuiKey_B );

    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport( )->GetCenter( );
    ImGui::SetNextWindowPos( center, ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );

    if ( ImGui::BeginPopupModal( LanguageProvider[ "Backpack" ], nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        const auto ModalStartPosition = ImGui::GetCursorPos( );

        static constexpr auto EchoCardSpacing = 15;
        static constexpr auto EchoImageSize   = 140;
        static constexpr auto SetImageSize    = 25;
        static constexpr auto CharImageSize   = 40;

        {
            ImGui::BeginChild( "EchoList", ImVec2( EchoImageSize * 6 + EchoCardSpacing * 5 + Style.WindowPadding.x * 2 + Style.ScrollbarSize, 700 ), ImGuiChildFlags_Border );

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2 { } );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2 { } );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2 { } );
            ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 3 );
            ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 3 );

            int X = 0, Y = 0;
            for ( int i = 0; i < m_Content.size( ); ++i )
            {
                auto& Echos = m_Content[ i ];
                ImGui::PushID( std::hash<std::string>( )( "BackPackEchoRect" ) + i );

                if ( X != 0 ) ImGui::SameLine( 0, EchoCardSpacing );

                {
                    if ( m_ContentAvailable[ i ] )
                        ImGui::PushStyleColor( ImGuiCol_Border, IM_COL32( 255, 255, 255, 255 ) );
                    else
                        ImGui::PushStyleColor( ImGuiCol_Border, IM_COL32( 255, 0, 0, 255 ) );
                    ImGui::BeginChild( "Card", ImVec2( EchoImageSize, EchoImageSize + SetImageSize ), ImGuiChildFlags_Border );
                    const auto ChildStartPos = ImGui::GetCursorPos( );
                    bool       Selected      = m_FocusEcho == i;
                    if ( ImGui::Selectable( "##Click", Selected, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_AllowDoubleClick, ImVec2( EchoImageSize, EchoImageSize + SetImageSize ) ) )
                    {
                        if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                        {
                            m_ContentAvailable[ i ] = !m_ContentAvailable[ i ];

                            Changed = true;
                            UpdateSelectedContent( );
                        } else
                        {
                            if ( !Selected )
                                m_FocusEcho = i;
                            else
                                m_FocusEcho = -1;
                        }
                    }

                    ImGui::SetCursorPos( ChildStartPos );
                    ImGui::Image( *OptimizerUIConfig::GetTextureOrDefault( Echos.EchoName ), sf::Vector2f { EchoImageSize, EchoImageSize } );
                    ImGui::Separator( );
                    ImGui::Image( *OptimizerUIConfig::GetTextureOrDefault( std::string( Echos.GetSetName( ) ) ), sf::Vector2f { SetImageSize, SetImageSize }, EchoSetSFColor[ (int) Echos.Set ] );

                    if ( !Echos.Occupation.empty( ) )
                    {
                        ImGui::PushStyleColor( ImGuiCol_ChildBg, IM_COL32( 255, 247, 134, 255 ) );
                        ImGui::PushStyleColor( ImGuiCol_Border, IM_COL32( 255, 247, 134, 255 ) );
                        ImGui::SetCursorPos( ChildStartPos + ImVec2 { 5, 5 } );
                        {
                            ImGui::BeginChild( "CharImg", ImVec2( CharImageSize, CharImageSize ), ImGuiChildFlags_Border );
                            const auto CharacterStartPosition = ImGui::GetCursorPos( );
                            if ( ImGui::Selectable( "Char##Click", Selected, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_AllowDoubleClick, ImVec2( CharImageSize, CharImageSize ) ) )
                            {
                                if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                                {
                                    Echos.Occupation.clear( );
                                    WriteToFile( );
                                }
                            }
                            ImGui::SetCursorPos( CharacterStartPosition );
                            ImGui::Image( *OptimizerUIConfig::GetTextureOrDefault( std::string( "SmallCharImg_" ) + Echos.Occupation ), sf::Vector2f { CharImageSize, CharImageSize } );
                            ImGui::EndChild( );
                        }
                        ImGui::PopStyleColor( 2 );
                    }

                    const auto LevelString = std::format( "+ {}", Echos.Level );
                    const auto LevelSize   = ImGui::CalcTextSize( LevelString.c_str( ) );
                    ImGui::SetCursorPos( { EchoImageSize - LevelSize.x - 5, EchoImageSize + ( SetImageSize - LevelSize.y ) / 2 } );
                    ImGui::Text( LevelString.c_str( ) );

                    ImGui::EndChild( );
                    ImGui::PopStyleColor( );
                }

                ++X;
                if ( X >= 6 )
                {
                    Y++;
                    X = 0;
                    ImGui::ItemSize( ImVec2( 0, EchoCardSpacing ) );
                }

                ImGui::PopID( );
            }
            ImGui::PopStyleVar( 5 );

            ImGui::EndChild( );
        }

        {
            ImGui::SameLine( );
            ImGui::BeginChild( "EchoDetails", ImVec2( EchoImageSize * 3 + Style.WindowPadding.x * 2, 700 ), ImGuiChildFlags_Border );
            if ( m_FocusEcho != -1 )
            {
                ImGui::Image( *OptimizerUIConfig::GetTextureOrDefault( m_Content[ m_FocusEcho ].EchoName ), sf::Vector2f { EchoImageSize, EchoImageSize } * 3.f );
                ImGui::Separator( );
                ImGui::Text( "%s", m_Content[ m_FocusEcho ].DetailStat( LanguageProvider ).c_str( ) );
            }
            ImGui::EndChild( );
        }

        {
            const auto         BackPackSize     = ImGui::GetWindowSize( );
            const auto         EndChildPosition = ImGui::GetCursorPos( );
            const sf::Vector2f SLIconSize       = sf::Vector2f( 30, 30 );
            const ImVec2       FilterImagePos   = { BackPackSize.x - SLIconSize.x - Style.WindowPadding.x - Style.FramePadding.x * 2, ModalStartPosition.y };

            ImGui::SetCursorPos( FilterImagePos );
            if ( ImGui::BeginChild( "ESSChi", ImVec2 { SLIconSize.x, SLIconSize.y } + Style.FramePadding * 2 ) )
            {
                if ( ImGui::ImageButton( *OptimizerUIConfig::GetTextureOrDefault( "Filter" ), SLIconSize ) )
                {
                    ImGui::OpenPopup( "EchoSetSelect" );
                }
            }

            if ( ImGui::BeginPopupContextWindow( "EchoSetSelect", ImGuiPopupFlags_NoReopen ) )
            {
                for ( int SetIndex = 0; SetIndex < (int) EchoSet::eEchoSetCount; ++SetIndex )
                {
                    const std::string ElementName { magic_enum::enum_name( (EchoSet) SetIndex ) };
                    ImGui::PushID( SetIndex + 1 );

                    if ( ImGui::Checkbox( "##CB", m_SetFilter.data( ) + SetIndex ) )
                    {
                        for ( int i = 0; i < m_Content.size( ); ++i )
                        {
                            if ( m_Content[ i ].Set == (EchoSet) SetIndex )
                            {
                                m_ContentAvailable[ i ] = m_SetFilter[ SetIndex ];
                                Changed                 = true;
                            }
                        }

                        UpdateSelectedContent( );
                    }

                    ImGui::SameLine( );
                    if ( !m_SetFilter[ SetIndex ] ) ImGui::BeginDisabled( );
                    const auto ElementImageSize = ( ImGui::GetFontSize( ) + Style.FramePadding.y * 2 );
                    ImGui::Image( *OptimizerUIConfig::GetTextureOrDefault( ElementName ), sf::Vector2f { ElementImageSize, ElementImageSize }, EchoSetSFColor[ SetIndex ] );
                    ImGui::SameLine( );
                    ImGui::Text( LanguageProvider[ ElementName ] );
                    if ( !m_SetFilter[ SetIndex ] ) ImGui::EndDisabled( );

                    ImGui::PopID( );
                }

                ImGui::EndPopup( );
            }

            ImGui::EndChild( );
            ImGui::SetCursorPos( EndChildPosition );
        }

        ImGui::Spacing( );
        ImGui::Separator( );
        ImGui::Spacing( );

        if ( ImGui::Button( LanguageProvider[ "Done" ], ImVec2( -1, 0 ) ) || BPressed )
        {
            ImGui::CloseCurrentPopup( );
        }
        ImGui::SetItemDefaultFocus( );

        ImGui::EndPopup( );
    } else if ( BPressed )
    {
        ImGui::OpenPopup( LanguageProvider[ "Backpack" ] );
    }

    return Changed;
}

void
Backpack::UpdateSelectedContent( )
{
    auto SelectionResult =
        m_Content
        | std::views::filter( [ this ]( FullStats& C ) -> bool {
              return m_ContentAvailable[ &C - m_Content.data( ) ];
          } );

    m_SelectedContent.clear( );
    std::ranges::copy( SelectionResult, std::back_inserter( m_SelectedContent ) );

    ++m_Hash;
}

void
Backpack::CharacterUnEquipEchoes( const std::string& CharacterName )
{
    for ( auto& Echo : m_Content )
    {
        if ( Echo.Occupation == CharacterName )
        {
            Echo.Occupation = "";
        }
    }

    WriteToFile( );
}

void
Backpack::CharacterEquipEchoes( const std::string& CharacterName, std::vector<int> EchoIndices )
{
    auto SelectionResult =
        m_Content
        | std::views::filter( [ this ]( FullStats& C ) -> bool {
              return m_ContentAvailable[ &C - m_Content.data( ) ];
          } );
    auto SelectionResultIt = SelectionResult.begin( );

    int EchoesIndex = 0;
    for ( int SelectedIndex : EchoIndices )
    {
        while ( EchoesIndex != SelectedIndex )
        {
            SelectionResultIt++;
            EchoesIndex++;
        }

        if ( !SelectionResultIt->Occupation.empty( ) )
        {
            spdlog::warn( "Echo was own by {}, replace by {}", SelectionResultIt->Occupation, CharacterName );
        }

        SelectionResultIt->Occupation = CharacterName;
    }

    WriteToFile( );
}

void
Backpack::WriteToFile( ) const
{
    std::ofstream File( m_EchoesPath );
    if ( File.is_open( ) )
    {
        YAML::Node ResultYAMLEchos { m_Content };
        File << ResultYAMLEchos;
        File.close( );
    } else
    {
        spdlog::error( "Unable to open {} for writing", m_EchoesPath );
    }
}

void
Backpack::RefreshEchoBan( )
{
    for ( int i = 0; i < m_Content.size( ); ++i )
    {
        m_ContentAvailable[ i ] = m_SetFilter[ (int) m_Content[ i ].Set ];
    }

    UpdateSelectedContent( );
}

void
Backpack::BanEquippedEchoesExcept( std::string& CharacterName )
{
    for ( int i = 0; i < m_Content.size( ); ++i )
    {
        m_ContentAvailable[ i ] = m_ContentAvailable[ i ] && ( m_Content[ i ].Occupation.empty( ) || m_Content[ i ].Occupation == CharacterName );
    }

    UpdateSelectedContent( );
}
