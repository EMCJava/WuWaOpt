//
// Created by EMCJava on 8/2/2024.
//

#pragma once

#include <Loca/Loca.hpp>
#include <Opt/FullStats.hpp>

#include <vector>
#include <ranges>

class Backpack : public LanguageObserver
{
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

protected:
    std::vector<FullStats> m_Content;
    std::vector<bool>      m_ContentAvailable;

    int m_FocusEcho = -1;
};
