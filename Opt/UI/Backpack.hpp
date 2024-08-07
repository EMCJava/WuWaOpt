//
// Created by EMCJava on 8/2/2024.
//

#pragma once

#include <Common/Stat/FullStats.hpp>
#include <Common/Stat/EchoSet.hpp>

#include <Loca/Loca.hpp>

#include <vector>
#include <vector>
#include <ranges>

class Backpack : public LanguageObserver
{
private:
    void UpdateSelectedContent( );

public:
    Backpack( const std::string& EchoesPath, const std::map<std::string, std::vector<std::string>>& EchoNamesBySet, Loca& LocaObj );

    void WriteToFile( ) const;

    void RefreshEchoBan( );
    void BanEquippedEchoesExcept( std::string& CharacterName );

    void CharacterEquipEchoes( const std::string& CharacterName, std::vector<int> EchoIndices );
    void CharacterUnEquipEchoes( const std::string& CharacterName );

    void Set( auto&& Stats )
    {
        m_Content.clear( );
        std::ranges::copy( Stats, std::back_inserter( m_Content ) );

        m_ContentAvailable.resize( m_Content.size( ), true );
        m_FocusEcho = -1;

        m_Hash = 0;
        UpdateSelectedContent( );
    }

    /*
     *
     * Return true if content changed
     *
     * */
    bool DisplayBackpack( );

    [[nodiscard]] std::size_t GetHash( ) const noexcept { return m_Hash; }
    [[nodiscard]] const auto& GetSelectedContent( ) const noexcept { return m_SelectedContent; }

protected:
    std::string m_EchoesPath;

    std::vector<FullStats> m_Content;
    std::vector<bool>      m_ContentAvailable;

    std::array<bool, (int) EchoSet::eEchoSetCount> m_SetFilter { true };

    std::vector<FullStats> m_SelectedContent;

    int         m_FocusEcho = -1;
    std::size_t m_Hash      = 0;
};

class Backpack;
template <>
struct std::hash<Backpack> {
    std::size_t operator( )( const Backpack& BP ) const
    {
        return BP.GetHash( );
    }
};
