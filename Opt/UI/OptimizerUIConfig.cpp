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

    LoadTexture( "TextureMissing", "data/texture_missing.png" );
    m_DefaultTexture = m_TextureCache[ "TextureMissing" ].get( );
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

std::optional<sf::Texture*>
OptimizerUIConfig::LoadTexture( const std::string& Names, const std::string& Path, const std::array<int, 4>& Rect )
{
    auto& Texture = m_Instance->m_TextureCache[ Names ];
    if ( !( Texture = std::make_unique<sf::Texture>( ) )->loadFromFile( Path, reinterpret_cast<const sf::IntRect&>( Rect ) ) )
    {
        spdlog::info( "Failed to load texture [{}]: {}", Names, Path );
        Texture.reset( );
        return std::nullopt;
    } else
    {
        Texture->generateMipmap( );
        return Texture.get( );
    }
}

std::optional<sf::Texture*>
OptimizerUIConfig::GetTexture( const std::string& Names )
{
    const auto TextureIt = m_Instance->m_TextureCache.find( Names );
    if ( TextureIt != m_Instance->m_TextureCache.end( ) )
    {
        return TextureIt->second.get( );
    }

    return std::nullopt;
}

sf::Texture*
OptimizerUIConfig::GetTextureDefault( )
{
    return m_Instance->m_DefaultTexture;
}

sf::Texture*
OptimizerUIConfig::GetTextureOrDefault( const std::string& Names )
{
    return GetTexture( Names ).value_or( GetTextureDefault( ) );
}
