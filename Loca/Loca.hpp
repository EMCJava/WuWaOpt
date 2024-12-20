//
// Created by EMCJava on 7/21/2024.
//

#pragma once

#include <unordered_map>
#include <string>
#include <list>

#include <yaml-cpp/yaml.h>

enum class Language {
    English,
    SimplifiedChinese,
    Undefined,
    LanguageSize = Undefined
};

namespace YAML
{
template <>
struct convert<Language> {
    static Node encode( const Language& rhs )
    {
        Node node;
        switch ( rhs )
        {
        case Language::English:
            node = "English";
            break;
        case Language::SimplifiedChinese:
            node = "SimplifiedChinese";
            break;
        default:
            node = "Undefined";
        }
        return node;
    }

    static bool decode( const Node& node, Language& rhs )
    {
        if ( node.as<std::string>( ) == "English" )
            rhs = Language::English;
        else if ( node.as<std::string>( ) == "SimplifiedChinese" )
            rhs = Language::SimplifiedChinese;
        else
            rhs = Language::Undefined;

        return true;
    }
};
}   // namespace YAML

class LanguageObserver;
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

    void AttachObserver( LanguageObserver* observer );
    void DetachObserver( LanguageObserver* observer );

protected:
    StringDecodedType m_EmptyDecodedString;
    std::string       m_EmptyRawString;

    std::unordered_map<std::string, std::pair<StringDecodedType, std::string>> m_StringData;

    Language m_Lang;

    std::list<LanguageObserver*> m_Observers;
};

class LanguageObserver
{
protected:
    Loca& LanguageProvider;

public:
    explicit LanguageObserver( Loca& LocaObj )
        : LanguageProvider( LocaObj )
    {
        LanguageProvider.AttachObserver( this );
    }
    ~LanguageObserver( )
    {
        LanguageProvider.DetachObserver( this );
    }
    virtual void OnLanguageChanged( Loca* ) { }
};
