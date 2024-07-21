//
// Created by EMCJava on 7/21/2024.
//

#include "Loca.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>
#include <codecvt>

std::string
ToLanguageCode( Language Lang )
{
    static_assert( static_cast<int>( Language::LanguageSize ) == 2 );

    switch ( Lang )
    {
    case Language::English:
        return "en-US";
    case Language::SimplifiedChinese:
        return "zh-CN";
    default:
        spdlog::warn( "Invalid language specified" );
        return "en-US";   // Default to English if invalid language is provided
    }
}

Language
FindSystemLanguage( )
{
    std::wstring name( LOCALE_NAME_MAX_LENGTH, '\0' );
    GetUserDefaultLocaleName( name.data( ), LOCALE_NAME_MAX_LENGTH );

    spdlog::info( L"Detected locale: {}", name );
    if ( name.starts_with( L"zh" ) )
    {
        return Language::SimplifiedChinese;
    }

    if ( name.starts_with( L"en" ) )
    {
        return Language::English;
    }

    return Language::Undefined;
}

Loca::Loca( Language Lang )
{
    if ( Lang == Language::Undefined ) Lang = FindSystemLanguage( );
    LoadLanguage( Lang );
}

void
Loca::LoadLanguage( )
{
    const auto LanguageCode = ToLanguageCode( m_Lang );
    setlocale( LC_ALL, LanguageCode.c_str( ) );
    spdlog::info( "Loading language file for {}", LanguageCode );

    std::wifstream InFile( std::format( "data/loca/{}.json", LanguageCode ) );
    if ( !InFile )
    {
        spdlog::error( "Failed to open language file for {}", LanguageCode );
        if ( m_Lang != Language::English )
        {
            spdlog::warn( "Falling back to English" );
            LoadLanguage( Language::English );
            return;
        }
        return;
    }

    InFile.imbue( std::locale( std::locale::empty( ), new std::codecvt_utf8<wchar_t> ) );
    std::wstringstream wss;
    wss << InFile.rdbuf( );

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    const auto&                                            Json = nlohmann::json::parse( wss.str( ) );
    for ( auto& [ key, val ] : Json[ "strings" ].items( ) )
    {
        try
        {
            const auto& Str     = val.get<std::string>( );
            m_StringData[ key ] = std::make_pair( converter.from_bytes( Str ), Str );
        }
        catch ( const std::exception& e )
        {
            spdlog::error( "Failed to parse string [{}]: {}", key, e.what( ) );
            continue;   // Continue to load the rest of the strings if one fails to parse
        }
    }
}

void
Loca::SetLanguage( Language Lang )
{
    m_Lang = Lang;
}

void
Loca::LoadLanguage( Language Lang )
{
    SetLanguage( Lang );
    LoadLanguage( );
}

const Loca::StringDecodedType&
Loca::GetDecodedString( const std::string& Key ) const noexcept
{
    const auto It = m_StringData.find( Key );
    if ( It != m_StringData.end( ) )
    {
        return It->second.first;
    }

    spdlog::error( "Failed to find key [{}]", Key );
    return m_EmptyDecodedString;
}

const std::string&
Loca::GetString( const std::string& Key ) const noexcept
{
    const auto It = m_StringData.find( Key );
    if ( It != m_StringData.end( ) )
    {
        return It->second.second;
    }

    spdlog::error( "Failed to find key [{}]", Key );
    return m_EmptyRawString;
}
