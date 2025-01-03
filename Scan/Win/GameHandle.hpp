//
// Created by EMCJava on 7/15/2024.
//

#pragma once

#include <windows.h>

#include <opencv2/core/core.hpp>

#include "MouseControl.hpp"

struct ResolutionRuntimeError {
    int ActualWidth, ActualHeight;
    int DesiredWidth, DesiredHeight;
};

class GameHandle
{
public:
    GameHandle( );
    ~GameHandle( );

    [[nodiscard]] MouseControl::MousePoint
    GetLeftTop( ) const;

    [[nodiscard]] MouseControl::MousePoint
    GetMousePointInGameSpace( ) const;

    cv::Mat ScreenCap( );

protected:
    void SelectGameWindow( );

    HWND m_TargetWindowHandle = nullptr;

    RECT    m_WindowSize { };
    HDC     m_CaptureDC     = nullptr;
    HDC     m_ScreenDC      = nullptr;
    HBITMAP m_CaptureBitmap = nullptr;
};
