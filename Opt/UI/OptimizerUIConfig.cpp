//
// Created by EMCJava on 7/31/2024.
//

#include "OptimizerUIConfig.hpp"

#include <imgui.h>

#include <imgui-SFML.h>
#include <SFML/Graphics/Texture.hpp>

#include <spdlog/spdlog.h>

OptimizerUIConfig* OptimizerUIConfig::m_Instance = nullptr;
OptimizerUIConfig::OptimizerUIConfig( Loca& LocaObj )
    : LanguageObserver( LocaObj )
{
    if ( m_Instance != nullptr )
    {
        throw std::runtime_error( "OptimizerUIConfig instance already exists!" );
    }

    m_Instance = this;
    SetupFont( );
    OptimizerUIConfig::OnLanguageChanged( &LanguageProvider );
}

OptimizerUIConfig::~OptimizerUIConfig( )
{
}

void
OptimizerUIConfig::SetupFont( )
{
    ImGuiIO& io = ImGui::GetIO( );
    io.Fonts->Clear( );
    ImFontConfig DefaultConfig;
    DefaultConfig.OversampleH              = 1;
    DefaultConfig.OversampleV              = 1;
    DefaultConfig.PixelSnapH               = true;
    DefaultConfig.SizePixels               = 13;
    m_EnglishFont[ FontSizeType::Default ] = io.Fonts->AddFontDefault( &DefaultConfig );
    DefaultConfig.SizePixels               = 15;
    m_ChineseFont[ FontSizeType::Default ] = io.Fonts->AddFontFromFileTTF( "data/sim_chi.ttf", 15.0f, &DefaultConfig, io.Fonts->GetGlyphRangesChineseFull( ) );

    DefaultConfig.SizePixels           = 45;
    m_EnglishFont[ FontSizeType::Big ] = io.Fonts->AddFontDefault( &DefaultConfig );
    DefaultConfig.SizePixels           = 52;
    m_ChineseFont[ FontSizeType::Big ] = io.Fonts->AddFontFromFileTTF( "data/sim_chi.ttf", 52.0f, &DefaultConfig, io.Fonts->GetGlyphRangesChineseFull( ) );

    ImGui::SFML::UpdateFontTexture( );
}

void
OptimizerUIConfig::OnLanguageChanged( Loca* NewLanguage )
{
    switch ( NewLanguage->GetLanguage( ) )
    {
    case Language::English:
        m_ActiveFont = &m_EnglishFont;
        break;
    case Language::SimplifiedChinese:
        m_ActiveFont = &m_ChineseFont;
        break;
    default:
        spdlog::error( "Unknown language in OptimizerUIConfig::OnLanguageChanged()" );
        m_ActiveFont = &m_EnglishFont;
    }
}

void
OptimizerUIConfig::PushFont( OptimizerUIConfig::FontSizeType Type )
{
    ImGui::PushFont( m_Instance->m_ActiveFont->at( Type ) );
}

void
OptimizerUIConfig::LoadTexture( const std::string& Names, const std::string& Path )
{
    if ( !( m_Instance->m_TextureCache[ Names ] = std::make_unique<sf::Texture>( ) )->loadFromFile( Path ) )
    {
        spdlog::info( "Failed to load texture [{}]: {}", Names, Path );
    }
}

sf::Texture*
OptimizerUIConfig::GetTexture( const std::string& Names )
{
    const auto TextureIt = m_Instance->m_TextureCache.find( Names );
    if ( TextureIt != m_Instance->m_TextureCache.end( ) )
    {
        return TextureIt->second.get( );
    }

    return nullptr;
}
