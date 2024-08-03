//
// Created by EMCJava on 8/2/2024.
//

#include "Backpack.hpp"

#include <Opt/UI/OptimizerUIConfig.hpp>

#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_internal.h>

inline std::array<sf::Color, eEchoSetCount + 1> EchoSetSFColor {
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

void
Backpack::DisplayBackpack( )
{
    const auto& Style    = ImGui::GetStyle( );
    const auto  BPressed = ImGui::IsKeyPressed( ImGuiKey_B );

    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport( )->GetCenter( );
    ImGui::SetNextWindowPos( center, ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );

    if ( ImGui::BeginPopupModal( LanguageProvider[ "Backpack" ], nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        static constexpr auto EchoCardSpacing = 10;
        static constexpr auto EchoImageSize   = 80;
        static constexpr auto SetImageSize    = 20;

        {
            ImGui::BeginChild( "EchoList", ImVec2( EchoImageSize * 6 + EchoCardSpacing * 5 + Style.WindowPadding.x * 2 + Style.ScrollbarSize, 700 ), ImGuiChildFlags_Border );

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2 { } );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2 { } );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2 { } );
            int X = 0, Y = 0;
            for ( int i = 0; i < m_Content.size( ); ++i )
            {
                auto& Echos = m_Content[ i ];
                ImGui::PushID( std::hash<std::string>( )( "BackPackEchoRect" ) + i );

                if ( X != 0 ) ImGui::SameLine( 0, EchoCardSpacing );

                {
                    if ( m_ContentAvailable[ i ] )
                        ImGui::PushStyleColor( ImGuiCol_Border, IM_COL32( 0, 255, 0, 255 ) );
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
                        } else
                        {
                            if ( !Selected )
                                m_FocusEcho = i;
                            else
                                m_FocusEcho = -1;
                        }
                    }

                    ImGui::SetCursorPos( ChildStartPos );
                    ImGui::Image( *OptimizerUIConfig::GetTexture( Echos.EchoName ), sf::Vector2f { EchoImageSize, EchoImageSize } );
                    ImGui::Separator( );
                    ImGui::Image( *OptimizerUIConfig::GetTexture( SetToString( Echos.Set ) ), sf::Vector2f { SetImageSize, SetImageSize }, EchoSetSFColor[ Echos.Set ] );

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
            ImGui::PopStyleVar( 3 );

            ImGui::EndChild( );
        }

        {
            ImGui::SameLine( );
            ImGui::BeginChild( "EchoDetails", ImVec2( EchoImageSize * 5 + Style.WindowPadding.x * 2, 700 ), ImGuiChildFlags_Border );
            if ( m_FocusEcho != -1 )
            {
                ImGui::Image( *OptimizerUIConfig::GetTexture( m_Content[ m_FocusEcho ].EchoName ), sf::Vector2f { EchoImageSize, EchoImageSize } * 5.f );
                ImGui::Separator( );
                ImGui::Text( "%s", m_Content[ m_FocusEcho ].DetailStat( LanguageProvider ).c_str( ) );
            }
            ImGui::EndChild( );
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
}

void
Backpack::UpdateSelectedContent( )
{
    auto SelectionResult =
        m_Content
        | std::views::filter( [ this ]( auto& C ) -> bool {
              return m_ContentAvailable[ &C - m_Content.data( ) ];
          } );

    m_SelectedContent.clear( );
    std::ranges::copy( SelectionResult, std::back_inserter( m_SelectedContent ) );

    ++m_Hash;
}
