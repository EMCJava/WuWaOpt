//
// Created by EMCJava on 7/31/2024.
//

#pragma once

#include <Loca/Loca.hpp>

#include <memory>
#include <array>

namespace sf
{
class Texture;
}
struct ImFont;
class OptimizerUIConfig : public LanguageObserver
{
public:
    enum FontSizeType {
        Default,
        Big,
        Size
    };

    OptimizerUIConfig( Loca& LocaObj );
    ~OptimizerUIConfig( );

    void SetupFont( );

    static auto& GetInstance( )
    {
        return *m_Instance;
    }

    static ImFont* GetFont( FontSizeType Type = FontSizeType::Default )
    {
        return m_Instance->m_ActiveFont->at( static_cast<int>( Type ) );
    }

    static void PushFont( FontSizeType Type = FontSizeType::Default );

    static void         LoadTexture( const std::string& Names, const std::string& Path );
    static sf::Texture* GetTexture( const std::string& Names );

    void OnLanguageChanged( Loca* NewLanguage ) override;

protected:
    std::array<ImFont*, FontSizeType::Size>* m_ActiveFont = nullptr;
    std::array<ImFont*, FontSizeType::Size>  m_EnglishFont;
    std::array<ImFont*, FontSizeType::Size>  m_ChineseFont;

    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> m_TextureCache;

    static OptimizerUIConfig* m_Instance;
};
