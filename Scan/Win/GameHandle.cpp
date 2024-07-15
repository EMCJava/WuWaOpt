//
// Created by EMCJava on 7/15/2024.
//


#include <future>
#include <array>

#include "GameHandle.hpp"
#include <Psapi.h>

#include <spdlog/spdlog.h>

namespace Win
{
HWND
GetGameWindow( )
{

    static constexpr char DefaultCaption[] = "Activate the game window and press OK";
    auto                  iotaFuture       = std::async( std::launch::async,
                                                         []( ) {
                                      return MessageBox(
                                          nullptr,
                                          DefaultCaption,
                                          "Game Window Selection",
                                          MB_ICONQUESTION | MB_OKCANCEL | MB_TOPMOST );
                                  } );

    HWND MessageHWND;
    while ( !( MessageHWND = FindWindow( nullptr, "Game Window Selection" ) ) )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }

    HWND hLast              = nullptr;
    auto MessageCaptionHWND = FindWindowEx( MessageHWND, hLast, nullptr, DefaultCaption );
    if ( MessageCaptionHWND == nullptr ) throw std::runtime_error( "CaptionHWND not found" );

    std::array<char, 256> WindowTitle { 0 };
    HWND                  Foreground = nullptr;
    while ( iotaFuture.wait_for( std::chrono::milliseconds( 100 ) ) != std::future_status::ready )
    {
        auto FG = GetForegroundWindow( );
        if ( FG == MessageHWND ) continue;
        if ( ( Foreground = FG ) )
        {
            GetWindowText( Foreground, WindowTitle.data( ), WindowTitle.size( ) );
            SetWindowText( MessageCaptionHWND, WindowTitle.data( ) );
        }
    }

    return iotaFuture.get( ) == IDCANCEL ? nullptr : Foreground;
}

BOOL CALLBACK
EnumGameProc( HWND hwnd, LPARAM lParam )
{
    const int MAX_CLASS_NAME = 255;
    char      className[ MAX_CLASS_NAME ];

    GetClassName( hwnd, className, MAX_CLASS_NAME );

    if ( strcmp( className, "UnrealWindow" ) != 0 ) return TRUE;

    DWORD dwProcId = 0;
    GetWindowThreadProcessId( hwnd, &dwProcId );
    HANDLE hProc = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId );
    if ( hProc != nullptr )
    {
        char executablePath[ MAX_PATH ] { 0 };
        if ( GetModuleFileNameEx( hProc, nullptr, executablePath, sizeof( executablePath ) / sizeof( char ) ) )
        {
            if ( strstr( executablePath, "Wuthering Waves Game" ) != nullptr )
            {
                spdlog::info( "Found Wuthering Waves Game window: {}", executablePath );
                *(HWND*) lParam = hwnd;
                return FALSE;
            }
        }

        CloseHandle( hProc );
    }

    return TRUE;
}
}   // namespace Win

namespace CVUtils
{
cv::Mat
HBitmapToMat( HBITMAP hBitmap )
{
    BITMAP bmp;
    GetObject( hBitmap, sizeof( bmp ), &bmp );

    cv::Mat mat( bmp.bmHeight, bmp.bmWidth, CV_8UC4 );

    BITMAPINFOHEADER bi = { 0 };
    bi.biSize           = sizeof( BITMAPINFOHEADER );
    bi.biWidth          = bmp.bmWidth;
    bi.biHeight         = -bmp.bmHeight;   // negative to flip the image
    bi.biPlanes         = 1;
    bi.biBitCount       = 32;
    bi.biCompression    = BI_RGB;

    GetDIBits( GetDC( nullptr ), hBitmap, 0, bmp.bmHeight, mat.data, (BITMAPINFO*) &bi, DIB_RGB_COLORS );

    return mat;
}
}   // namespace CVUtils

GameHandle::GameHandle( )
{
    SelectGameWindow( );

    m_ScreenDC  = GetDC( GetDesktopWindow( ) );
    m_CaptureDC = CreateCompatibleDC( m_ScreenDC );   // create a device context to use yourself

    GetWindowRect( m_TargetWindowHandle, &m_WindowRect );
    m_CaptureBitmap = CreateCompatibleBitmap( m_ScreenDC, m_WindowRect.right - m_WindowRect.left, m_WindowRect.bottom - m_WindowRect.top );   // create a bitmap

    // use the previously created device context with the bitmap
    SelectObject( m_CaptureDC, m_CaptureBitmap );
}

void
GameHandle::SelectGameWindow( )
{
    EnumWindows( Win::EnumGameProc, reinterpret_cast<LPARAM>( &m_TargetWindowHandle ) );
    if ( m_TargetWindowHandle == nullptr )
    {
        m_TargetWindowHandle = Win::GetGameWindow( );
        if ( m_TargetWindowHandle == nullptr )
        {
            throw std::runtime_error( "Unable to find Wuthering Waves Game window" );
        }
    }

    std::wstring TargetWindowTitle( 256, L'\0' );
    GetWindowTextW( m_TargetWindowHandle, TargetWindowTitle.data( ), (int) TargetWindowTitle.size( ) );
    spdlog::info( L"Selected game window: {} - {}", fmt::ptr( m_TargetWindowHandle ), TargetWindowTitle.data( ) );

    SetForegroundWindow( m_TargetWindowHandle );
}

cv::Mat
GameHandle::ScreenCap( )
{
    // GetWindowRect( TargetWindowHandle, &m_WindowRect );

    BitBlt( m_CaptureDC,
            0, 0, m_WindowRect.right - m_WindowRect.left, m_WindowRect.bottom - m_WindowRect.top,
            m_ScreenDC,
            m_WindowRect.left, m_WindowRect.top, SRCCOPY );
    return CVUtils::HBitmapToMat( m_CaptureBitmap );
}

GameHandle::~GameHandle( )
{
    DeleteObject( m_CaptureBitmap );
    DeleteDC( m_CaptureDC );
}

MouseControl::MousePoint
GameHandle::GetLeftTop( ) const
{
    return { static_cast<float>( m_WindowRect.left ), static_cast<float>( m_WindowRect.top ) };
}