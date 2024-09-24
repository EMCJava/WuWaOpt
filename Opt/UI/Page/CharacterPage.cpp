//
// Created by EMCJava on 8/5/2024.
//

#include "CharacterPage.hpp"

#include <Opt/UI/OptimizerUIConfig.hpp>

#include <SFML/Graphics.hpp>

#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_internal.h>

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <ranges>

namespace ImGui
{

struct InputTextCallback_UserData {
    std::string*           Str;
    ImGuiInputTextCallback ChainCallback;
    void*                  ChainCallbackUserData;
};

static int
InputTextCallback( ImGuiInputTextCallbackData* data )
{
    InputTextCallback_UserData* user_data = (InputTextCallback_UserData*) data->UserData;
    if ( data->EventFlag == ImGuiInputTextFlags_CallbackResize )
    {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
        std::string* str = user_data->Str;
        IM_ASSERT( data->Buf == str->c_str( ) );
        str->resize( data->BufTextLen );
        data->Buf = (char*) str->c_str( );
    } else if ( user_data->ChainCallback )
    {
        // Forward to user callback, if any
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback( data );
    }
    return 0;
}

bool
InputText( const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr )
{
    IM_ASSERT( ( flags & ImGuiInputTextFlags_CallbackResize ) == 0 );
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str                   = str;
    cb_user_data.ChainCallback         = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputText( label, (char*) str->c_str( ), str->capacity( ) + 1, flags, InputTextCallback, &cb_user_data );
}
}   // namespace ImGui

#define SAVE_CONFIG( x ) \
    if ( x ) SaveCharacters( );

void
CharacterPage::DisplayStatConfigPopup( float WidthPerPanel )
{
    const auto   MainViewport = ImGui::GetMainViewport( );
    const ImVec2 Center       = MainViewport->GetCenter( );
    ImGui::SetNextWindowPos( Center, ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
    if ( ImGui::BeginPopupModal( LanguageProvider[ "StatsComposition" ], nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        ImGui::SetNextWindowSizeConstraints( ImVec2 { 0, 0 }, { MainViewport->Size.x * 0.75f, FLT_MAX } );
        ImGui::BeginChild( "StatsCompositionList", ImVec2( 0, 0 ), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_HorizontalScrollbar );

        float PenalHeight = 250;
        for ( auto& [ CompositionName, CompositionStats ] : m_ActiveCharacterConfig.GetStatsCompositions( ) )
        {
            ImGui::SameLine( );
            ImGui::BeginChild( reinterpret_cast<uint64_t>( &CompositionName ), ImVec2( WidthPerPanel, 0 ), ImGuiChildFlags_AutoResizeY );
            ImGui::PushID( &CompositionName );

            ImGui::SeparatorText( CompositionName.c_str( ) );
            SAVE_CONFIG( ImGui::InputText( LanguageProvider[ "CompositionName" ], &CompositionName ) );
            SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "FlatAttack" ], &CompositionStats.flat_attack, 1, 0, 0, "%.0f" ) )
            SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "Attack%" ], &CompositionStats.percentage_attack, 0.01, 0, 0, "%.2f" ) )
            SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "ElementBuff%" ], &CompositionStats.buff_multiplier, 0.01, 0, 0, "%.2f" ) )
            SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "AutoAttack%" ], &CompositionStats.auto_attack_buff, 0.01, 0, 0, "%.2f" ) )
            SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "HeavyAttack%" ], &CompositionStats.heavy_attack_buff, 0.01, 0, 0, "%.2f" ) )
            SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "SkillDamage%" ], &CompositionStats.skill_buff, 0.01, 0, 0, "%.2f" ) )
            SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "UltDamage%" ], &CompositionStats.ult_buff, 0.01, 0, 0, "%.2f" ) )
            SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "CritRate" ], &CompositionStats.crit_rate, 0.01, 0, 0, "%.2f" ) )
            SAVE_CONFIG( ImGui::DragFloat( LanguageProvider[ "CritDamage" ], &CompositionStats.crit_damage, 0.01, 0, 0, "%.2f" ) )
            ImGui::PopID( );
            PenalHeight = std::max( PenalHeight, ImGui::GetCursorPos( ).y );
            ImGui::Spacing( );
            ImGui::EndChild( );
        }

        static bool IsAppendCompositionHovered = false;
        ImGui::PushStyleColor( ImGuiCol_ChildBg, IsAppendCompositionHovered ? IM_COL32( 0, 204, 102, 230 ) : IM_COL32( 0, 255, 153, 230 ) );
        ImGui::SameLine( );
        ImGui::BeginChild( "AppendComposition", ImVec2( 15, PenalHeight ), ImGuiChildFlags_Border, ImGuiWindowFlags_NoDecoration );
        ImGui::EndChild( );
        IsAppendCompositionHovered = ImGui::IsItemHovered( );
        if ( ImGui::IsItemClicked( ) )
        {
            m_ActiveCharacterConfig.StatsCompositions.emplace_back( std::to_string( m_ActiveCharacterConfig.StatsCompositions.size( ) ) );
            SaveCharacters( );
        }

        ImGui::PopStyleColor( );


        ImGui::EndChild( );

        ImGui::Spacing( );
        ImGui::Separator( );
        ImGui::Spacing( );

        if ( ImGui::Button( LanguageProvider[ "Done" ], ImVec2( -1, 0 ) ) )
        {
            ImGui::CloseCurrentPopup( );
        }
        ImGui::SetItemDefaultFocus( );

        ImGui::EndPopup( );
    }
}

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
        m_CharactersNode      = YAML::LoadFile( CharacterFileName );
        CharacterConfigCaches = m_CharactersNode
            | std::views::transform( []( const YAML::const_iterator ::value_type& Node ) {
                                    return std::make_pair( Node.first.as<std::string>( ), std::get<1>( Node ).as<CharacterConfig>( ) );
                                } )
            | std::ranges::to<std::unordered_map>( );
    }

    for ( const auto& entry : std::filesystem::directory_iterator( "data/character_img" ) )
    {
        if ( entry.is_regular_file( ) )
        {
            const constexpr auto DefaultCharacterSize = 256;

            const auto Name = entry.path( ).stem( ).string( );
            OptimizerUIConfig::LoadTexture( "CharImg_" + Name, entry.path( ).string( ) );
            m_CharacterNames.push_back( Name );

            const constexpr double CharacterZoomFactor = 0.75;
            const constexpr int    SmallSize           = DefaultCharacterSize * CharacterZoomFactor;
            if ( auto SmallTexture =
                     OptimizerUIConfig::LoadTexture( "SmallCharImg_" + Name, entry.path( ).string( ),
                                                     {
                                                         ( DefaultCharacterSize - SmallSize ) / 2,
                                                         ( DefaultCharacterSize - SmallSize ) / 2,
                                                         SmallSize,
                                                         SmallSize,
                                                     } ) )
            {
                SmallTexture.value( )->generateMipmap( );
                SmallTexture.value( )->setSmooth( true );
            }
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
        m_ActiveCharacterConfig = { };

        // Load default config
        FromNode( YAML::Node { }, m_ActiveCharacterConfig );

        m_ActiveCharacterNode = m_ActiveCharacterConfig;
        m_ActiveSkillDisplay = m_ActiveDeepenDisplay = { };
        SaveCharacters( );
        return false;
    }

    m_ActiveCharacterConfig = m_ActiveCharacterNode.as<CharacterConfig>( );
    m_ActiveSkillDisplay    = {
           .auto_attack_multiplier  = m_ActiveCharacterConfig.SkillConfig.auto_attack_multiplier * 100,
           .heavy_attack_multiplier = m_ActiveCharacterConfig.SkillConfig.heavy_attack_multiplier * 100,
           .skill_multiplier        = m_ActiveCharacterConfig.SkillConfig.skill_multiplier * 100,
           .ult_multiplier          = m_ActiveCharacterConfig.SkillConfig.ult_multiplier * 100 };
    m_ActiveDeepenDisplay = {
        .auto_attack_multiplier  = m_ActiveCharacterConfig.DeepenConfig.auto_attack_multiplier * 100,
        .heavy_attack_multiplier = m_ActiveCharacterConfig.DeepenConfig.heavy_attack_multiplier * 100,
        .skill_multiplier        = m_ActiveCharacterConfig.DeepenConfig.skill_multiplier * 100,
        .ult_multiplier          = m_ActiveCharacterConfig.DeepenConfig.ult_multiplier * 100 };
    return true;
}


void
CharacterPage::SaveCharacters( )
{
    m_ActiveCharacterConfig.InternalStageID++;

    // Assume all changes will lead to SaveCharacters being called
    m_ActiveCharacterConfig.UpdateOverallStats( );
    m_ActiveCharacterNode = m_ActiveCharacterConfig;

    std::ofstream OutFile( CharacterFileName );
    OutFile << m_CharactersNode;
    OutFile.close( );
}

bool
CharacterPage::DisplayCharacterInfo( float Width, float* HeightOut )
{

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

    const auto* OverallStatsText       = LanguageProvider[ "OverallStats" ];
    const auto  OverallStatsTextHeight = ImGui::CalcTextSize( OverallStatsText ).y;

    ImGui::SeparatorTextEx( 0, OverallStatsText, OverallStatsText + strlen( OverallStatsText ), OverallStatsTextHeight + Style.SeparatorTextPadding.x );
    ImGui::SameLine( );
    if ( ImGui::ImageButton( *OptimizerUIConfig::GetTextureOrDefault( "Decomposition" ), { OverallStatsTextHeight, OverallStatsTextHeight } ) )
    {
        ImGui::OpenPopup( LanguageProvider[ "StatsComposition" ] );
    }
    DisplayStatConfigPopup( 350 );

    ImGui::PushID( "OverallStats" );
    ImGui::BeginDisabled( );
    ImGui::DragFloat( LanguageProvider[ "FlatAttack" ], const_cast<float*>( &m_ActiveCharacterConfig.GetOverallStats( ).flat_attack ), 1, 0, 0, "%.0f" );
    ImGui::DragFloat( LanguageProvider[ "Attack%" ], const_cast<float*>( &m_ActiveCharacterConfig.GetOverallStats( ).percentage_attack ), 0.01, 0, 0, "%.2f" );
    ImGui::DragFloat( LanguageProvider[ "ElementBuff%" ], const_cast<float*>( &m_ActiveCharacterConfig.GetOverallStats( ).buff_multiplier ), 0.01, 0, 0, "%.2f" );
    ImGui::DragFloat( LanguageProvider[ "AutoAttack%" ], const_cast<float*>( &m_ActiveCharacterConfig.GetOverallStats( ).auto_attack_buff ), 0.01, 0, 0, "%.2f" );
    ImGui::DragFloat( LanguageProvider[ "HeavyAttack%" ], const_cast<float*>( &m_ActiveCharacterConfig.GetOverallStats( ).heavy_attack_buff ), 0.01, 0, 0, "%.2f" );
    ImGui::DragFloat( LanguageProvider[ "SkillDamage%" ], const_cast<float*>( &m_ActiveCharacterConfig.GetOverallStats( ).skill_buff ), 0.01, 0, 0, "%.2f" );
    ImGui::DragFloat( LanguageProvider[ "UltDamage%" ], const_cast<float*>( &m_ActiveCharacterConfig.GetOverallStats( ).ult_buff ), 0.01, 0, 0, "%.2f" );
    ImGui::DragFloat( LanguageProvider[ "CritRate" ], const_cast<float*>( &m_ActiveCharacterConfig.GetOverallStats( ).crit_rate ), 0.01, 0, 0, "%.2f" );
    ImGui::DragFloat( LanguageProvider[ "CritDamage" ], const_cast<float*>( &m_ActiveCharacterConfig.GetOverallStats( ).crit_damage ), 0.01, 0, 0, "%.2f" );
    ImGui::EndDisabled( );
    ImGui::PopID( );
    ImGui::EndChild( );

    /*********************************
     *          Empty Place          *
     *********************************/

    ImGui::NewLine( );
    ImGui::Separator( );
    ImGui::NewLine( );

#define SAVE_MULTIPLIER_CONFIG( name, type, stat )                                                                \
    if ( ImGui::DragFloat( name, &m_Active##type##Display.stat##_multiplier ) )                                   \
    {                                                                                                             \
        m_ActiveCharacterConfig.type##Config.stat##_multiplier = m_Active##type##Display.stat##_multiplier / 100; \
        SaveCharacters( );                                                                                        \
    }

    {
        ImGui::BeginChild( "ConfigPanel##Deepen", ImVec2( Width / 2 - Style.WindowPadding.x * 4, 0 ), ImGuiChildFlags_AutoResizeY );
        ImGui::SeparatorText( LanguageProvider[ "DMGDeep%" ] );
        ImGui::PushID( "Deepen" );
        SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "AutoTotal%" ], Deepen, auto_attack )
        SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "HeavyTotal%" ], Deepen, heavy_attack )
        SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "SkillTotal%" ], Deepen, skill )
        SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "UltTotal%" ], Deepen, ult )
        ImGui::PopID( );
        ImGui::EndChild( );
        ImGui::SameLine( );
        ImGui::BeginChild( "ConfigPanel##Skill", ImVec2( Width / 2 - Style.WindowPadding.x * 4, 0 ), ImGuiChildFlags_AutoResizeY );
        ImGui::SeparatorText( LanguageProvider[ "CycleTotal%" ] );
        ImGui::PushID( "Skill" );
        SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "AutoTotal%" ], Skill, auto_attack )
        SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "HeavyTotal%" ], Skill, heavy_attack )
        SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "SkillTotal%" ], Skill, skill )
        SAVE_MULTIPLIER_CONFIG( LanguageProvider[ "UltTotal%" ], Skill, ult )
        ImGui::PopID( );
        ImGui::EndChild( );
    }
#undef SAVE_MULTIPLIER_CONFIG

    ImGui::NewLine( );
    ImGui::Separator( );
    ImGui::NewLine( );

    ImGui::PushItemWidth( -200 );

    SAVE_CONFIG( ImGui::Combo( LanguageProvider[ "ElementType" ],
                               (int*) &m_ActiveCharacterConfig.CharacterElement,
                               m_ElementLabels.GetRawStrings( ),
                               m_ElementLabels.GetStringCount( ) ) )

    ImGui::PopItemWidth( );

    if ( HeightOut ) *HeightOut = ImGui::GetWindowHeight( );
    ImGui::EndChild( );

    return Changed;
}
