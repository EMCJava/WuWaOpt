//
// Created by EMCJava on 8/2/2024.
//

#include "Backpack.hpp"

#include <Opt/UI/UIConfig.hpp>
#include <Opt/UI/Page/CharacterPage.hpp>

#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_internal.h>

#include <magic_enum/magic_enum.hpp>

#include <spdlog/spdlog.h>

#include <fstream>

Backpack::Backpack( const std::string& EchoesPath, const std::map<std::string, std::vector<std::string>>& EchoNamesBySet, CharacterPage& CharacterPageConfig, Loca& LocaObj )
    : LanguageObserver( LocaObj )
    , m_CharacterConfigRef( CharacterPageConfig )
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

    // Set occupation names
    std::multimap<std::size_t, std::string> EchoAssignments;
    for ( auto& [ Name, CharConfig ] : m_CharacterConfigRef.GetCharacters( ) )
    {
        CharConfig.RuntimeEquippedEchoName.resize( CharConfig.EquippedEchoHashes.size( ) );
        for ( auto& EquippedEchoHash : CharConfig.EquippedEchoHashes )
            EchoAssignments.insert( { EquippedEchoHash, Name } );
    }

    for ( auto& Echo : FullStatsList )
    {
        if ( const auto AssignmentIt = EchoAssignments.find( Echo.EchoHash ); AssignmentIt != EchoAssignments.end( ) )
        {
            auto& EquippedBy = m_CharacterConfigRef.GetCharacter( AssignmentIt->second );

            const auto EchoSlotIndex = std::distance( EquippedBy.EquippedEchoHashes.begin( ), std::ranges::find( EquippedBy.EquippedEchoHashes, Echo.EchoHash ) );

            EquippedBy.RuntimeEquippedEchoName[ EchoSlotIndex ] = Echo.EchoName;

            Echo.RuntimeOccupation = AssignmentIt->second;
            EchoAssignments.erase( AssignmentIt );
        }
    }

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
                auto& CurrentEcho = m_Content[ i ];
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
                    ImGui::Image( *UIConfig::GetTextureOrDefault( CurrentEcho.EchoName ), sf::Vector2f { EchoImageSize, EchoImageSize } );
                    ImGui::Separator( );
                    ImGui::Image( *UIConfig::GetTextureOrDefault( std::string( CurrentEcho.GetSetName( ) ) ), sf::Vector2f { SetImageSize, SetImageSize } );

                    if ( !CurrentEcho.RuntimeOccupation.empty( ) )
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
                                    EchoUnEquip( CurrentEcho );
                                }
                            }
                            ImGui::SetCursorPos( CharacterStartPosition );
                            ImGui::Image( *UIConfig::GetTextureOrDefault( std::string( "SmallCharImg_" ) + CurrentEcho.RuntimeOccupation ), sf::Vector2f { CharImageSize, CharImageSize } );
                            ImGui::EndChild( );
                        }
                        ImGui::PopStyleColor( 2 );
                    }

                    const auto LevelString = std::format( "+ {}", CurrentEcho.Level );
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
                ImGui::Image( *UIConfig::GetTextureOrDefault( m_Content[ m_FocusEcho ].EchoName ), sf::Vector2f { EchoImageSize, EchoImageSize } * 3.f );
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
                if ( ImGui::ImageButton( "FilterImageButton", *UIConfig::GetTextureOrDefault( "Filter" ), SLIconSize ) )
                {
                    ImGui::OpenPopup( "EchoSetSelect" );
                }
            }

            if ( ImGui::BeginPopupContextWindow( "EchoSetSelect", ImGuiPopupFlags_NoReopen ) )
            {
                for ( int SetIndex = 0; SetIndex < (int) EchoSet::eEchoSetCount; ++SetIndex )
                {
                    const std::string SetName { magic_enum::enum_name( (EchoSet) SetIndex ) };
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
                    ImGui::Image( *UIConfig::GetTextureOrDefault( SetName ), sf::Vector2f { ElementImageSize, ElementImageSize } );
                    ImGui::SameLine( );
                    ImGui::Text( LanguageProvider[ SetName ] );
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
    } else if ( BPressed && !ImGui::GetIO( ).WantCaptureKeyboard )
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
        if ( Echo.RuntimeOccupation == CharacterName )
        {
            EchoUnEquip( Echo, false );
        }
    }

    m_CharacterConfigRef.SaveCharacter( CharacterName );
    WriteToFile( );
}

void
Backpack::EchoUnEquip( FullStats& Echo, bool DoWrite ) const
{
    auto&      EquippingCharacter = m_CharacterConfigRef.GetCharacter( Echo.RuntimeOccupation );
    const auto TargetEchoIt       = std::ranges::find( EquippingCharacter.EquippedEchoHashes, Echo.EchoHash );
    if ( TargetEchoIt != EquippingCharacter.EquippedEchoHashes.end( ) )
    {
        const auto EchoSlotIndex = std::distance( EquippingCharacter.EquippedEchoHashes.begin( ), TargetEchoIt );
        EquippingCharacter.RuntimeEquippedEchoName.erase( EquippingCharacter.RuntimeEquippedEchoName.begin( ) + EchoSlotIndex );
        EquippingCharacter.EquippedEchoHashes.erase( TargetEchoIt );
        if ( DoWrite ) m_CharacterConfigRef.SaveCharacter( Echo.RuntimeOccupation );

        Echo.RuntimeOccupation.clear( );
    } else
    {
        spdlog::info( "Echos was not equipped by character {}", Echo.RuntimeOccupation );
    }
}

void
Backpack::EchoEquip( const std::string& CharacterName, FullStats& Echo, bool DoWrite ) const
{
    if ( !Echo.RuntimeOccupation.empty( ) )
    {
        spdlog::warn( "Echo was own by {}, replace by {}", Echo.RuntimeOccupation, CharacterName );
    }

    auto& TargetCharacter = m_CharacterConfigRef.GetCharacter( CharacterName );
    if ( TargetCharacter.EquippedEchoHashes.size( ) < 5 )
    {
        TargetCharacter.RuntimeEquippedEchoName.push_back( Echo.EchoName );
        TargetCharacter.EquippedEchoHashes.push_back( Echo.EchoHash );
        Echo.RuntimeOccupation = CharacterName;
        if ( DoWrite ) m_CharacterConfigRef.SaveCharacter( CharacterName );
    } else
    {
        spdlog::warn( "Character has equipped all five slot, ignore #{}", Echo.EchoHash );
    }
}

void
Backpack::CharacterEquipEchoes( const std::string& CharacterName, const std::vector<int>& EchoIndices )
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

        EchoEquip( CharacterName, *SelectionResultIt, false );
    }

    m_CharacterConfigRef.SaveCharacter( CharacterName );
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
        m_ContentAvailable[ i ] = m_ContentAvailable[ i ] && ( m_Content[ i ].RuntimeOccupation.empty( ) || m_Content[ i ].RuntimeOccupation == CharacterName );
    }

    UpdateSelectedContent( );
}
