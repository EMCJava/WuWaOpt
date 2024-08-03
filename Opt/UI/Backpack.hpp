//
// Created by EMCJava on 8/2/2024.
//

#pragma once

#include <Loca/Loca.hpp>
#include <Opt/FullStats.hpp>

#include <vector>
#include <vector>
#include <ranges>

class Backpack : public LanguageObserver
{
private:
    void UpdateSelectedContent( );

public:
    using LanguageObserver::LanguageObserver;

    void Set( auto&& Stats )
    {
        m_Content.clear( );
        std::ranges::copy( Stats, std::back_inserter( m_Content ) );

        m_ContentAvailable.resize( m_Content.size( ), true );
        m_FocusEcho = -1;
    }

    void DisplayBackpack( );

    [[nodiscard]] std::size_t GetHash( ) const noexcept { return m_Hash; }
    [[nodiscard]] const auto& GetSelectedContent( ) const noexcept { return m_SelectedContent; }

protected:
    std::vector<FullStats> m_Content;
    std::vector<bool>      m_ContentAvailable;

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
