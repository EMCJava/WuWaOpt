//
// Created by EMCJava on 7/21/2024.
//

#pragma once

#include <unordered_map>
#include <string>

enum class Language {
    English,
    SimplifiedChinese,
    Undefined,
    LanguageSize = Undefined
};

class Loca
{
    using StringDecodedType = std::wstring;

public:
    using DecodedStringConstPtr = StringDecodedType::const_pointer;

    Loca( Language Lang = Language::Undefined );

    void LoadLanguage( );
    void LoadLanguage( Language Lang );
    void SetLanguage( Language Lang );

    [[nodiscard]] const StringDecodedType& GetDecodedString( const std::string& Key ) const noexcept;
    [[nodiscard]] const std::string&       GetString( const std::string& Key ) const noexcept;

    [[nodiscard]] auto* operator[]( const std::string& Key ) const noexcept
    {
        return GetString( Key ).data( );
    }

    [[nodiscard]] Language GetLanguage( ) const noexcept { return m_Lang; }

protected:
    StringDecodedType m_EmptyDecodedString;
    std::string       m_EmptyRawString;

    std::unordered_map<std::string, std::pair<StringDecodedType, std::string>> m_StringData;

    Language m_Lang;
};