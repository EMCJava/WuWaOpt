//
// Created by EMCJava on 8/5/2024.
//

#pragma once

#include <Opt/Config/CharacterConfig.hpp>

#include <yaml-cpp/yaml.h>

#include <vector>
#include <string>

class CharacterPage
{
    static constexpr char CharacterFileName[] = "characters.yaml";

public:
    CharacterPage( );

    [[nodiscard]] std::vector<std::string> GetCharacterList( ) const;

    bool LoadCharacter( const std::string& CharacterName );
    void SaveCharacters( );

    [[nodiscard]] auto& GetActiveConfig( ) { return m_ActiveCharacterConfig; }

protected:
    YAML::Node m_CharactersNode;

    std::string     m_ActiveCharacterName;
    YAML::Node      m_ActiveCharacterNode;
    CharacterConfig m_ActiveCharacterConfig;
};
