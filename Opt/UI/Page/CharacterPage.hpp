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

public:
    CharacterPage( Loca& LocaObj );

    auto& GetActiveCharacterNames( ) { return m_ActiveCharacterName; }

    bool LoadCharacter( const std::string& CharacterName );
    void SaveCharacters( );

    [[nodiscard]] std::vector<std::string> GetCharacterList( ) const;
    [[nodiscard]] auto&                    GetActiveConfig( ) { return m_ActiveCharacterConfig; }

    // Return true if active character changed
    bool DisplayCharacterInfo( float Width, float* HeightOut = nullptr );

protected:
    std::vector<std::string> m_CharacterNames;
    YAML::Node               m_CharactersNode { };

    std::string           m_ActiveCharacterName;
    YAML::Node            m_ActiveCharacterNode;
    CharacterConfig       m_ActiveCharacterConfig;
    SkillMultiplierConfig m_ActiveSkillDisplay;
    SkillMultiplierConfig m_ActiveDeepenDisplay;

    // UIs
    StringArrayObserver m_ElementLabels;
};
