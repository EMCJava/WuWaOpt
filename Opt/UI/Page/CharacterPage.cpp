//
// Created by EMCJava on 8/5/2024.
//

#include "CharacterPage.hpp"

#include <Opt/UI/OptimizerUIConfig.hpp>

#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_internal.h>

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <ranges>

CharacterPage::CharacterPage( Loca& LocaObj )
    : LanguageObserver( LocaObj )
    , m_ElementLabels( LocaObj,
                       { "FireDamage",
                         "AirDamage",
                         "IceDamage",
                         "ElectricDamage",
                         "DarkDamage",
                         "LightDamage" } )
{
    if ( std::filesystem::exists( CharacterFileName ) )
    {
        m_CharactersNode = YAML::LoadFile( CharacterFileName );
    }

    for ( const auto& entry : std::filesystem::directory_iterator( "data/character_img" ) )
    {
        if ( entry.is_regular_file( ) )
        {
            const auto Name = entry.path( ).stem( ).string( );
            OptimizerUIConfig::LoadTexture( "CharImg_" + Name, entry.path( ).string( ) );
            m_CharacterNames.push_back( Name );
        }
    }

    // Merge two name set
    std::ranges::for_each( m_CharactersNode,
                           [ this ]( const YAML::const_iterator ::value_type& Node ) {
                               const auto  Name    = Node.first.as<std::string>( );
                               const auto& Profile = Node.second[ "Profile" ];
                               if ( Profile )
                               {
                                   // Replace by user defined texture
                                   OptimizerUIConfig::LoadTexture( "CharImg_" + Name, Profile.as<std::string>( ) );
                               }
                               if ( !std::ranges::contains( m_CharacterNames, Name ) )
                                   m_CharacterNames.push_back( Name );
                           } );

    if ( m_CharacterNames.empty( ) )
    {
        spdlog::error( "No character image found, character list empty" );
    } else
    {
        LoadCharacter( m_CharacterNames.front( ) );
    }
}

std::vector<std::string>
CharacterPage::GetCharacterList( ) const
{
    return m_CharactersNode
        | std::views::transform( []( const YAML::const_iterator::value_type& Node ) {
               return Node.first.as<std::string>( );
           } )
        | std::ranges::to<std::vector>( );
}

bool
CharacterPage::LoadCharacter( const std::string& CharacterName )
{
    m_ActiveCharacterName = CharacterName;
    m_ActiveCharacterNode.reset( m_CharactersNode[ m_ActiveCharacterName ] );
    if ( !m_ActiveCharacterNode )
    {
        m_ActiveCharacterNode = m_ActiveCharacterConfig = { };
        m_ActiveSKillMultiplierDisplay                  = { };
        SaveCharacters( );
        return false;
    }

    m_ActiveCharacterConfig        = m_ActiveCharacterNode.as<CharacterConfig>( );
    m_ActiveSKillMultiplierDisplay = {
        .auto_attack_multiplier  = m_ActiveCharacterConfig.SkillMultiplierConfig.auto_attack_multiplier * 100,
        .heavy_attack_multiplier = m_ActiveCharacterConfig.SkillMultiplierConfig.heavy_attack_multiplier * 100,
        .skill_multiplier        = m_ActiveCharacterConfig.SkillMultiplierConfig.skill_multiplier * 100,
        .ult_multiplier          = m_ActiveCharacterConfig.SkillMultiplierConfig.ult_multiplier * 100 };
    return true;
}


void
CharacterPage::SaveCharacters( )
{
    m_ActiveCharacterConfig.InternalStageID++;
    m_ActiveCharacterNode = m_ActiveCharacterConfig;

    std::ofstream OutFile( CharacterFileName );
    OutFile << m_CharactersNode;
    OutFile.close( );
}

bool
CharacterPage::DisplayCharacterInfo( float Width, float* HeightOut )
{
#define SAVE_CONFIG( x ) \
    if ( x ) SaveCharacters( );

    bool        Changed = false;
    const auto& Style   = ImGui::GetStyle( );

    ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 5.0f );
    ImGui::BeginChild( "ConfigPanel", ImVec2( Width - Style.WindowPadding.x * 4, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize );

    const auto DisplayImageWithSize = []( const std::string& ImageName, float ImageSize ) {
        const auto StartPos = ImGui::GetCursorPos( );
        const auto Texture  = OptimizerUIConfig::GetTexture( "CharImg_" + ImageName );
        if ( Texture.has_value( ) )
        {
            ImGui::Image( *Texture.value( ), sf::Vector2f { ImageSize, ImageSize } );
        } else
        {
            ImGui::Image( *OptimizerUIConfig::GetTextureDefault( ), sf::Vector2f { ImageSize, ImageSize } );
            ImGui::SetCursorPos( StartPos );
            ImGui::Text( "%s", ImageName.c_str( ) );
        }
    };

    {
        static float   ConfigHeight = 50;
        constexpr auto ImageSpace   = 20;

        ImGui::BeginChild( "StaticPanel##Config", ImVec2( ( Width - ConfigHeight - ImageSpace ) - Style.WindowPadding.x * 8, 0 ), ImGuiChildFlags_AutoResizeY );
        ImGui::SeparatorText( LanguageProvider[ "StaticConfig" ] );
        ImGui::PushItemWidth( -ImGui::CalcTextSize( LanguageProvider[ "ElResistance" ] ).x - Style.FramePadding.x );
        SAVE_CONFIG( ImGui::SliderInt( LanguageProvider[ "CharLevel" ], &m_ActiveCharacterConfig.CharacterLevel, 1, 90 ) )
        SAVE_CONFIG( ImGui::SliderInt( LanguageProvider[ "EnemyLevel" ], &m_ActiveCharacterConfig.EnemyLevel, 1, 90 ) )
        SAVE_CONFIG( ImGui::SliderFloat( LanguageProvider[ "ElResistance" ], &m_ActiveCharacterConfig.ElementResistance, 0, 1, "%.2f" ) )
        SAVE_CONFIG( ImGui::SliderFloat( LanguageProvider[ "DamReduction" ], &m_ActiveCharacterConfig.ElementDamageReduce, 0, 1, "%.2f" ) )
        ImGui::PopItemWidth( );
        ConfigHeight = ImGui::GetWindowHeight( );
        ImGui::EndChild( );

        ImGui::SameLine( 0, ImageSpace );

        ImGui::BeginChild( "StaticPanel##Image", ImVec2( ConfigHeight, ConfigHeight ), ImGuiChildFlags_AutoResizeY );
        {
            const auto ChildStartPos = ImGui::GetCursorPos( );
            if ( ImGui::Selectable( "##Click", false, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_AllowDoubleClick, ImVec2( ConfigHeight, ConfigHeight ) ) )
            {
                ImGui::OpenPopup( LanguageProvider[ "CharSel" ] );
            }
            ImGui::SetCursorPos( ChildStartPos );
            DisplayImageWithSize( m_ActiveCharacterName, ConfigHeight );
        }

        ImGui::SetNextWindowPos( ImGui::GetMainViewport( )->GetCenter( ), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );
        if ( ImGui::BeginPopupModal( LanguageProvider[ "CharSel" ], nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize ) )
        {
            static constexpr auto CharacterImgSpacing = 20;
            static constexpr auto CharacterImgSize    = 150;

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2 { } );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2 { } );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2 { } );
            int X = 0, Y = 0;
            for ( int i = 0; i < m_CharacterNames.size( ); ++i )
            {
                auto& Name = m_CharacterNames[ i ];
                ImGui::PushID( std::hash<std::string>( )( "CharacterPick" ) + i );

                if ( X != 0 ) ImGui::SameLine( 0, CharacterImgSpacing );

                {
                    ImGui::BeginChild( "CharacterCard", ImVec2( CharacterImgSize, CharacterImgSize ), ImGuiChildFlags_Border );
                    const auto ChildStartPos = ImGui::GetCursorPos( );
                    if ( ImGui::Selectable( "##Click", m_ActiveCharacterName == Name, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_AllowDoubleClick, ImVec2( CharacterImgSize, CharacterImgSize ) ) )
                    {
                        Changed = true;
                        LoadCharacter( m_ActiveCharacterName = Name );
                        ImGui::CloseCurrentPopup( );
                    }
                    ImGui::SetCursorPos( ChildStartPos );
                    DisplayImageWithSize( Name, CharacterImgSize );
                    ImGui::EndChild( );
                }

                ++X;
                if ( X >= 6 )
                {
                    Y++;
                    X = 0;
                    ImGui::ItemSize( ImVec2( 0, CharacterImgSpacing ) );
                }

                ImGui::PopID( );
            }
            ImGui::PopStyleVar( 3 );

            ImGui::Spacing( );
            ImGui::Separator( );
            ImGui::Spacing( );

            if ( ImGui::Button( LanguageProvider[ "Cancel" ], ImVec2( -1, 0 ) ) )
            {
                ImGui::CloseCurrentPopup( );
            }
            ImGui::SetItemDefaultFocus( );
            ImGui::EndPopup( );
        }

        ImGui::EndChild( );
    }

    ImGui::Separator( );

    ImGui::BeginChild( "ConfigPanel##Character", ImVec2( Width / 2 - Style.WindowPadding.x * 4, 0 ), ImGuiChildFlags_AutoResizeY );
    ImGui::SeparatorText( LanguageProvider[ "Character" ] );
    ImGui::PushID( "CharacterStat" );
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "FlatAttack" ], &m_ActiveCharacterConfig.CharacterStats.flat_attack, 1, 0, 0, "%.0f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "Attack%" ], &m_ActiveCharacterConfig.CharacterStats.percentage_attack, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "ElementBuff%" ], &m_ActiveCharacterConfig.CharacterStats.buff_multiplier, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "AutoAttack%" ], &m_ActiveCharacterConfig.CharacterStats.auto_attack_buff, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "HeavyAttack%" ], &m_ActiveCharacterConfig.CharacterStats.heavy_attack_buff, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "SkillDamage%" ], &m_ActiveCharacterConfig.CharacterStats.skill_buff, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "UltDamage%" ], &m_ActiveCharacterConfig.CharacterStats.ult_buff, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "CritRate" ], &m_ActiveCharacterConfig.CharacterStats.crit_rate, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "CritDamage" ], &m_ActiveCharacterConfig.CharacterStats.crit_damage, 0.01, 0, 0, "%.2f" ) )
    ImGui::PopID( );
    ImGui::EndChild( );
    ImGui::SameLine( );
    ImGui::BeginChild( "ConfigPanel##Weapon", ImVec2( Width / 2 - Style.WindowPadding.x * 4, 0 ), ImGuiChildFlags_AutoResizeY );
    ImGui::SeparatorText( LanguageProvider[ "Weapon" ] );
    ImGui::PushID( "WeaponStat" );
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "FlatAttack" ], &m_ActiveCharacterConfig.WeaponStats.flat_attack, 1, 0, 0, "%.0f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "Attack%" ], &m_ActiveCharacterConfig.WeaponStats.percentage_attack, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "ElementBuff%" ], &m_ActiveCharacterConfig.WeaponStats.buff_multiplier, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "AutoAttack%" ], &m_ActiveCharacterConfig.WeaponStats.auto_attack_buff, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "HeavyAttack%" ], &m_ActiveCharacterConfig.WeaponStats.heavy_attack_buff, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "SkillDamage%" ], &m_ActiveCharacterConfig.WeaponStats.skill_buff, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "UltDamage%" ], &m_ActiveCharacterConfig.WeaponStats.ult_buff, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "CritRate" ], &m_ActiveCharacterConfig.WeaponStats.crit_rate, 0.01, 0, 0, "%.2f" ) )
    SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "CritDamage" ], &m_ActiveCharacterConfig.WeaponStats.crit_damage, 0.01, 0, 0, "%.2f" ) )
    ImGui::PopID( );
    ImGui::EndChild( );

    ImGui::NewLine( );
    ImGui::Separator( );
    ImGui::NewLine( );

    ImGui::PushItemWidth( -200 );

#define SAVE_MULTIPLIER_CONFIG( name, stat )                                                            \
    if ( ImGui::DragFloat( name, &m_ActiveSKillMultiplierDisplay.stat ) )                               \
    {                                                                                                   \
        m_ActiveCharacterConfig.SkillMultiplierConfig.stat = m_ActiveSKillMultiplierDisplay.stat / 100; \
        SaveCharacters( );                                                                              \
    }
    SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "AutoTotal%" ], auto_attack_multiplier )
    SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "HeavyTotal%" ], heavy_attack_multiplier )
    SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "SkillTotal%" ], skill_multiplier )
    SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "UltTotal%" ], ult_multiplier )
#undef SAVE_MULTIPLIER_CONFIG

    ImGui::NewLine( );
    ImGui::Separator( );
    ImGui::NewLine( );

    SAVE_CONFIG( ImGui::Combo( LanguageProvider[ "ElementType" ],
                               (int*) &m_ActiveCharacterConfig.CharacterElement,
                               m_ElementLabels.GetRawStrings( ),
                               m_ElementLabels.GetStringCount( ) ) )

    ImGui::PopItemWidth( );

    if ( HeightOut ) *HeightOut = ImGui::GetWindowHeight( );
    ImGui::EndChild( );

    return Changed;
#undef SAVE_CONFIG
}
