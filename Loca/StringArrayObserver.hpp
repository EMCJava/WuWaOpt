//
// Created by EMCJava on 7/26/2024.
//

#pragma once

#include "Loca.hpp"

#include <ranges>

class StringArrayObserver : public LanguageObserver
{
public:
    explicit StringArrayObserver( Loca& Language );
    StringArrayObserver( Loca& Language, std::initializer_list<const char*> Strings );

    void OnLanguageChanged( Loca* ) override;

    void SetKeyStrings( std::initializer_list<const char*>&& Strings )
    {
        SetKeyStrings( Strings );
    }

    void SetKeyStrings( auto&& Strings )
    {
        KeyStrings.clear( );
        std::ranges::copy( Strings, std::back_inserter( KeyStrings ) );
        OnLanguageChanged( &LanguageProvider );
    }

    const char** GetRawStrings( ) { return ValueStrings.data( ); }
    auto         GetStringCount( ) { return ValueStrings.size( ); }

protected:
    std::vector<const char*> KeyStrings;
    std::vector<const char*> ValueStrings;
};
