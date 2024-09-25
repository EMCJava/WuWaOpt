//
// Created by EMCJava on 8/5/2024.
//

#pragma once

#include <Loca/Loca.hpp>
#include <Loca/StringArrayObserver.hpp>

#include <Opt/Config/CharacterConfig.hpp>

#include <yaml-cpp/yaml.h>

#include <vector>
#include <string>

class CharacterPage : public LanguageObserver
{
    static constexpr char CharacterFileName[] = "characters.yaml";

    void DisplayStatConfigPopup( float WidthPerPanel );

public:
    CharacterPage( Loca& LocaObj );

    auto& GetActiveCharacterNames( ) { return m_ActiveCharacterName; }

    auto&            GetCharacters( ) noexcept { return CharacterConfigCaches; };
    CharacterConfig& GetCharacter( const std::string& CharacterName );
    void             LoadCharacter( const std::string& CharacterName );
    void             SaveCharacter( const std::string& CharacterName );
    void             SaveActiveCharacter( );

    [[nodiscard]] std::vector<std::string> GetCharacterList( ) const;
    [[nodiscard]] auto&                    GetActiveConfig( ) { return *m_ActiveCharacterConfig; }

    // Return true if active character changed
    bool DisplayCharacterInfo( float Width, float* HeightOut = nullptr );

protected:
    std::vector<std::string> m_CharacterNames;
    YAML::Node               m_CharactersNode { };

    std::unordered_map<std::string, CharacterConfig> CharacterConfigCaches;

    std::string           m_ActiveCharacterName;
    CharacterConfig*      m_ActiveCharacterConfig;
    SkillMultiplierConfig m_ActiveSkillDisplay;
    SkillMultiplierConfig m_ActiveDeepenDisplay;

    // UIs
    StringArrayObserver m_ElementLabels;
};
