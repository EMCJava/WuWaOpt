//
// Created by EMCJava on 7/26/2024.
//

#include "StringArrayObserver.hpp"

#include <ranges>

void
StringArrayObserver::OnLanguageChanged( Loca* Location )
{
    ValueStrings =
        KeyStrings
        | std::views::transform(
            [ Location ]( const std::string& Key ) {
                return ( *Location )[ Key ];
            } )
        | std::ranges::to<std::vector>( );
}

StringArrayObserver::StringArrayObserver( Loca& Language )
    : LanguageObserver( Language )
{
}

StringArrayObserver::StringArrayObserver( Loca& Language, std::initializer_list<const char*> Strings )
    : StringArrayObserver( Language )
{
    KeyStrings = Strings;
    StringArrayObserver::OnLanguageChanged( &LanguageProvider );
}