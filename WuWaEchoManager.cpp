//
// Created by EMCJava on 12/30/2024.
//

#include <Opt/UI/UIConfig.hpp>

#include <imgui.h>        // necessary for ImGui::*, imgui-SFML.h doesn't include imgui.h
#include <imgui-SFML.h>   // for ImGui::SFML::* functions and SFML-specific overloads

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <Common/ImGuiUtil.hxx>

#include <spdlog/spdlog.h>

struct EchoManagerConfig {
    std::string EchoCountStrInput;
};

int
main( )
{
    spdlog::set_level( spdlog::level::trace );
    srand( time( nullptr ) );

    Loca LanguageProvider;
    spdlog::info( "Using language: {}", LanguageProvider[ "Name" ] );

    sf::RenderWindow window( sf::VideoMode( 1500, 1000 ), LanguageProvider.GetDecodedString( "WinTitle" ) );
    window.setFramerateLimit( 60 );
    if ( !ImGui::SFML::Init( window, false ) ) return -1;

    UIConfig UIConfig( LanguageProvider );

    EchoManagerConfig ManagerConfig;

    sf::Clock DeltaClock;
    while ( window.isOpen( ) )
    {
        sf::Event event { };
        while ( window.pollEvent( event ) )
        {
            ImGui::SFML::ProcessEvent( window, event );

            if ( event.type == sf::Event::Closed )
            {
                window.close( );
            }
        }

        ImGui::SFML::Update( window, DeltaClock.restart( ) );
        UIConfig.PushFont( );

        ImGui::ShowDemoWindow( );

        if ( ImGui::Begin( "Manager Config" ) )
        {
            ImGui::InputText( "Echo Count", &ManagerConfig.EchoCountStrInput );
        }
        ImGui::End( );

        ImGui::PopFont( );
        window.clear( );
        ImGui::SFML::Render( window );
        window.display( );
    }

    ImGui::SFML::Shutdown( );

    return 0;
}