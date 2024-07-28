//
// Created by EMCJava on 7/15/2024.
//

#pragma once

#include <filesystem>
#include <valarray>
#include <random>

class MouseControl
{
public:
    using MousePoint      = std::valarray<float>;
    using MouseTrailArray = std::vector<MousePoint>;

    MouseControl( const std::filesystem::path& Path );

    void LoadMouseTrails( const std::filesystem::path& Path );

    void MoveMouse( const MousePoint& Start, const MousePoint& End, int MoveMilliseconds = 1000 );

    static void     LeftDown( );
    static void     LeftUp( );
    void            LeftClick( int ClickCount = 1, int Millisecond = -1 );
    static uint32_t ScrollMouse( int Delta );

protected:
    std::vector<MouseTrailArray> m_MouseTrails;

    std::random_device               RandomDevice;
    std::mt19937                     RandGenerator;
    std::uniform_real_distribution<> MouseDist;
};
