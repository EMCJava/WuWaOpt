//
// Created by EMCJava on 7/15/2024.
//

#include "MouseControl.hpp"

#include <fstream>
#include <ranges>
#include <thread>

#include <windows.h>

MouseControl::MouseControl( const std::filesystem::path& Path )
    : RandGenerator( RandomDevice( ) )
    , MouseDist( 0, 1 )
{
    LoadMouseTrails( Path );
}

void
MouseControl::LoadMouseTrails( const std::filesystem::path& Path )
{
    using namespace std::ranges;
    using std::views::chunk;
    using std::views::filter;
    using std::views::transform;

    if ( !std::filesystem::exists( Path ) )
    {
        throw std::runtime_error( "Mouse trail file not found" );
    }

    std::ifstream MouseTrailInput( Path );

    std::size_t TrailsCount, TrailResolution;
    MouseTrailInput >> TrailsCount >> TrailResolution;

    m_MouseTrails =
        istream_view<float>( MouseTrailInput )
        | chunk( TrailResolution * /* x and y coordinate */ 2 )
        | transform( []( auto&& r ) {
              return r
                  | chunk( 2 )
                  | transform( []( auto&& r ) {
                         auto iter = r.begin( );
                         return MousePoint { *r.begin( ), *++iter };
                     } )
                  | to<std::vector>( );
          } )
        | to<std::vector>( )
        | filter( []( auto&& Trail ) {
              return all_of( Trail, []( auto&& MP ) {
                  return MP[ 0 ] < 1.2 && MP[ 1 ] < 1.2;
              } );
          } )
        | to<std::vector>( );

    for ( auto& Trail : m_MouseTrails )
        Trail.back( ) = { 1, 1 };
}

void
MouseControl::MoveMouse( const MouseControl::MousePoint& Start,
                         const MouseControl::MousePoint& End,
                         int                             MoveMilliseconds )
{
    const auto& MouseTrail = m_MouseTrails[ rand( ) % m_MouseTrails.size( ) ];

    const auto StartTime       = std::chrono::high_resolution_clock::now( );
    const auto MouseMoveOffset = End - Start;
    for ( int i = 0; i < MouseTrail.size( ) - 1; ++i )
    {
        const auto Coordinate = ( Start + MouseMoveOffset * MouseTrail[ i ] ).apply( std::round );

        SetCursorPos( Coordinate[ 0 ], Coordinate[ 1 ] );

        auto EndTime = StartTime + std::chrono::microseconds( ( i == 0 ? 0 : i - 1 ) * MoveMilliseconds * 1000 / MouseTrail.size( ) );
        if ( i != 0 ) EndTime += std::chrono::microseconds( int( MouseDist( RandGenerator ) * MoveMilliseconds * 1000 / MouseTrail.size( ) ) );
        std::this_thread::sleep_until( EndTime );
    }

    SetCursorPos( End[ 0 ], End[ 1 ] );
}

void
MouseControl::LeftDown( )
{
    INPUT Input      = { 0 };
    Input.type       = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    ::SendInput( 1, &Input, sizeof( INPUT ) );
}

void
MouseControl::LeftUp( )
{
    INPUT Input      = { 0 };
    Input.type       = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    ::SendInput( 1, &Input, sizeof( INPUT ) );
}

void
MouseControl::LeftClick( int ClickCount, int Millisecond )
{
    LeftDown( );
    if ( Millisecond == -1 ) Millisecond = 200 + 40 * ( MouseDist( RandGenerator ) - 0.5 );
    std::this_thread::sleep_for( std::chrono::microseconds( Millisecond ) );
    LeftUp( );

    if ( ClickCount > 1 )
    {
        if ( Millisecond == -1 ) Millisecond = 200 + 40 * ( MouseDist( RandGenerator ) - 0.5 );
        std::this_thread::sleep_for( std::chrono::microseconds( Millisecond ) );
        LeftClick( ClickCount - 1, Millisecond );
    }
}

uint32_t
MouseControl::ScrollMouse( int Delta )
{
    INPUT InputEvent;
    POINT MousePosition;
    GetCursorPos( &MousePosition );

    InputEvent.type           = INPUT_MOUSE;
    InputEvent.mi.dwFlags     = MOUSEEVENTF_WHEEL;
    InputEvent.mi.time        = NULL;            // Windows will do the timestamp
    InputEvent.mi.mouseData   = (DWORD) Delta;   // A positive value indicates that the wheel was rotated forward, away from the user; a negative value indicates that the wheel was rotated backward, toward the user. One wheel click is defined as WHEEL_DELTA, which is 120.
    InputEvent.mi.dx          = MousePosition.x;
    InputEvent.mi.dy          = MousePosition.y;
    InputEvent.mi.dwExtraInfo = GetMessageExtraInfo( );

    return ::SendInput( 1, &InputEvent, sizeof( INPUT ) );
}
