//
// Created by EMCJava on 5/28/2024.
//

#define NOMINMAX
#include <windows.h>
#include <winrt/Windows.Foundation.h>

#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <ranges>

#include <Common/Stat/FullStats.hpp>

#include <Scan/BackpackEchoScanner.hpp>

#include <Loca/Loca.hpp>

#include <spdlog/spdlog.h>

#include <yaml-cpp/yaml.h>
bool
CheckResolution( )
{
    const POINT ptZero  = { 0, 0 };
    HMONITOR    monitor = MonitorFromPoint( ptZero, MONITOR_DEFAULTTOPRIMARY );

    MONITORINFOEX info = { sizeof( MONITORINFOEX ) };
    winrt::check_bool( GetMonitorInfo( monitor, &info ) );
    DEVMODE devmode = { };
    devmode.dmSize  = sizeof( DEVMODE );
    winrt::check_bool( EnumDisplaySettings( info.szDevice, ENUM_CURRENT_SETTINGS, &devmode ) );
    return devmode.dmPelsWidth == info.rcMonitor.right && devmode.dmPelsHeight == info.rcMonitor.bottom;
}

int
AskTotalEcho( const Loca& LanguageProvider )
{
    int EchoLeftToScan;
    spdlog::info( LanguageProvider[ "AskForNumEcho" ] );
    std::cin >> EchoLeftToScan;

    if ( EchoLeftToScan < 18 )
    {
        spdlog::error( LanguageProvider[ "EchoLessThanReq" ] );
        system( "pause" );
        exit( 1 );
    }

    return EchoLeftToScan;
}

int
AskScanDelay( const Loca& LanguageProvider )
{
    int ScanDelay;
    spdlog::info( LanguageProvider[ "AskForScanDelay" ] );
    std::cin >> ScanDelay;

    if ( ScanDelay < 0 )
    {
        spdlog::error( LanguageProvider[ "WrongScanDelay" ] );
        system( "pause" );
        exit( 1 );
    }

    return ScanDelay;
}

int
main( )
{
    spdlog::set_level( spdlog::level::trace );
    spdlog::set_pattern( "[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] %v" );
    srand( time( nullptr ) );

    Loca LanguageProvider;
    spdlog::info( "Using language: {}", LanguageProvider[ "Name" ] );

    if ( !CheckResolution( ) )
    {
        spdlog::error( LanguageProvider[ "ResolutionMismatch" ] );
        system( "pause" );
        return 1;
    }

    const int  TotalEcho = AskTotalEcho( LanguageProvider );
    const int  ScanDelay = AskScanDelay( LanguageProvider );
    const bool StopAtUnEscalated =
        MessageBox(
            nullptr,
            LanguageProvider.GetDecodedString( "StopScanQuestion" ).data( ),
            LanguageProvider.GetDecodedString( "ScannerBeh" ).data( ),
            MB_ICONQUESTION | MB_YESNO | MB_TOPMOST )
        == IDYES;

    YAML::Node ResultYAMLEchos = { };
    spdlog::info( "Scanning {} echoes...", TotalEcho );

    BackpackEchoScanner Scanner( LanguageProvider );
    Scanner.SetScanDelay( ScanDelay );
    Scanner.Scan( TotalEcho, [ & ]( const auto& FS ) {
        if ( StopAtUnEscalated && FS.Level == 0 ) return false;

        ResultYAMLEchos.push_back( FS );
        return true;
    } );

    std::ofstream OutputJson( "echoes.yaml" );
    OutputJson << ResultYAMLEchos << std::endl;

    system( "pause" );
}