//
// Created by EMCJava on 8/5/2024.
//

#include "CharacterPage.hpp"

#include <fstream>
#include <ranges>


CharacterPage::CharacterPage( )
{
    m_CharactersNode = YAML::LoadFile( CharacterFileName );
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
    m_ActiveCharacterNode = m_CharactersNode[ m_ActiveCharacterName ];
    if ( !m_ActiveCharacterNode )
    {
        m_ActiveCharacterNode = m_ActiveCharacterConfig = { };
        SaveCharacters( );
        return false;
    }

    m_ActiveCharacterConfig = m_ActiveCharacterNode.as<CharacterConfig>( );
    return true;
}


void
CharacterPage::SaveCharacters( )
{
    std::ofstream OutFile( CharacterFileName );
    OutFile << m_CharactersNode;
    OutFile.close( );
}
