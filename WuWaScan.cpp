//
// Created by EMCJava on 5/28/2024.
//

#define NOMINMAX
#include <windows.h>
#include <winrt/Windows.Foundation.h>

#include <semaphore>
#include <chrono>
#include <fstream>
#include <future>
#include <numeric>
#include <queue>
#include <random>
#include <ranges>
#include <utility>
#include <vector>
#include <memory>

#include <Common/Stat/FullStats.hpp>

#include <Scan/Win/MouseControl.hpp>
#include <Scan/Win/GameHandle.hpp>
#include <Scan/Recognizer/EchoExtractor.hpp>

#include <Opt/OptUtil.hpp>

#include <Loca/Loca.hpp>

#include <spdlog/spdlog.h>

#include <yaml-cpp/yaml.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/ml/ml.hpp>

std::random_device               rd;
std::mt19937                     gen( rd( ) );
std::uniform_real_distribution<> dis( 0.3, 0.7 );

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

namespace GameCoordinateConfigs
{
MouseControl::MousePoint ScanLeftTop      = { 205, 120 };
MouseControl::MousePoint ScanRightBottom  = { 1185, 915 };
FloatTy                  SpaceToNextCardX = 166.8;
FloatTy                  SpaceToNextCardY = 205.6;
MouseControl::MousePoint CardSpacing      = { SpaceToNextCardX, SpaceToNextCardY };
FloatTy                  CardWidth        = 147;
FloatTy                  CardHeight       = 178;
MouseControl::MousePoint CardSize         = { CardWidth, CardHeight };
}   // namespace GameCoordinateConfigs

int
main( )
{
    spdlog::set_level( spdlog::level::trace );
    srand( time( nullptr ) );

    Loca LanguageProvider;
    spdlog::info( "Using language: {}", LanguageProvider[ "Name" ] );

    if ( !CheckResolution( ) )
    {
        spdlog::error( LanguageProvider[ "ResolutionMismatch" ] );
        system( "pause" );
        return 1;
    }

    int EchoLeftToScan;
    spdlog::info( LanguageProvider[ "AskForNumEcho" ] );
    std::cin >> EchoLeftToScan;

    if ( EchoLeftToScan < 18 )
    {
        spdlog::error( LanguageProvider[ "EchoLessThanReq" ] );
        system( "pause" );
        return 1;
    }

    const bool StopAtUnEscalated =
        MessageBox(
            nullptr,
            LanguageProvider.GetDecodedString( "StopScanQuestion" ).data( ),
            LanguageProvider.GetDecodedString( "ScannerBeh" ).data( ),
            MB_ICONQUESTION | MB_YESNO | MB_TOPMOST )
        == IDYES;

    std::unique_ptr<GameHandle> GameHandler;

    try
    {
        GameHandler = std::make_unique<GameHandle>( );
    }
    catch ( const ResolutionRuntimeError& e )
    {
        spdlog::error( std::vformat( LanguageProvider[ "WrongWinSize" ], std::make_format_args( e.ActualWidth, e.ActualHeight, e.DesiredWidth, e.DesiredHeight ) ) );
        system( "pause" );
        return 1;
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Error initializing game handle: [{}] {}", e.what( ), LanguageProvider[ e.what( ) ] );
        system( "pause" );
        return 1;
    }

    spdlog::info( "Scanning {} echoes...", EchoLeftToScan );

    MouseControl::MousePoint MouseLocation( 2 );

    YAML::Node ResultYAMLEchos = { };

    std::vector<cv::Point> ListOfCardIndex;
    for ( int j = 0; j < 3; ++j )
        for ( int i = 0; i < 6; ++i )
            ListOfCardIndex.emplace_back( i, j );

    MouseControl MouseController( "data/MouseTrail.txt" );
    const auto   MouseToCard = [ & ]( int X, int Y ) {
        using namespace GameCoordinateConfigs;
        MouseControl::MousePoint NextLocation = CardSize;
        NextLocation *= (float) dis( gen );
        NextLocation += GameHandler->GetLeftTop( ) + ScanLeftTop + ( CardSpacing * MouseControl::MousePoint { (FloatTy) X, (FloatTy) Y } );

        MouseController.MoveMouse( MouseLocation, NextLocation, 300 + 80 * ( dis( gen ) - 0.5 ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

        MouseLocation = NextLocation;
    };

    EchoExtractor Extractor;
    const auto    ReadCard = [ & ]( ) {
        spdlog::info( "Reading card..." );
        Stopwatch SW;
        int       Retry;
        for ( Retry = 0; Retry < 3; ++Retry )
        {
            try
            {
                const auto FS = Extractor.ReadCard( GameHandler->ScreenCap( ) );
                spdlog::trace( "\n{}", YAML::Dump( YAML::Node( FS ) ) );
                ResultYAMLEchos.push_back( FS );

                if ( StopAtUnEscalated && FS.Level == 0 )
                {
                    return false;
                }

                return true;
            }
            catch ( const std::exception& e )
            {
                spdlog::error( "Error reading card: [{}] {}", e.what( ), LanguageProvider[ e.what( ) ] );
                std::this_thread::sleep_for( std::chrono::milliseconds( 400 ) );
            }
        }

        time_t t = std::time( nullptr );
        tm     buf { };
        localtime_s( &buf, &t );

        std::stringstream FileName;
        FileName << std::put_time( &buf, "%d_%m_%Y_%H_%M_%S" ) << "_defective.png";

        cv::imwrite( FileName.str( ), GameHandler->ScreenCap( ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );

        spdlog::error( "Unable to reading card after {} attempts", Retry );
        return true;
    };

    // Make sure it is sorted by echo level
    {
        MouseControl::MousePoint ClickLocation { 435, 980 };
        ClickLocation += GameHandler->GetLeftTop( );

        MouseController.MoveMouse( MouseLocation, ClickLocation, 300 + 80 * ( dis( gen ) - 0.5 ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
        MouseController.LeftClick( );

        MouseLocation = { 435, 690 };
        MouseLocation += GameHandler->GetLeftTop( );
        MouseController.MoveMouse( ClickLocation, MouseLocation, 300 + 80 * ( dis( gen ) - 0.5 ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
        MouseController.LeftClick( 3 );
    }

    //    while ( true )
    //    {
    //        {
    //            Stopwatch  SW;
    //            const auto FS = Extractor.ReadCard( GameHandler->ScreenCap( ) );
    //        }
    //        cv::waitKey( 1 );
    //    }

    bool                  Terminate = false;
    std::binary_semaphore CardReading { 1 };
    const auto            ReadCardInLocations = [ & ]( auto&& Location ) {
        MouseToCard( Location[ 0 ].x, Location[ 0 ].y );
        MouseController.LeftClick( 3 );

        for ( const auto& CardLocation : Location | std::views::drop( 1 ) )
        {
            std::thread { [ & ] {
                CardReading.acquire( );
                std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
                Terminate = Terminate || !ReadCard( );
                CardReading.release( );
            } }.detach( );

            MouseToCard( CardLocation.x, CardLocation.y );

            CardReading.acquire( );
            if ( Terminate || GetAsyncKeyState( VK_SPACE ) )
            {
                Terminate = true;
                return;
            }
            MouseController.LeftClick( 3 );
            CardReading.release( );
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
        ReadCard( );   // Read the final card
    };

    for ( int Page = 0;; ++Page )
    {
        spdlog::info( "Scanning at page {}...", Page );

        ReadCardInLocations( ListOfCardIndex );
        if ( Terminate ) break;

        for ( int i = 0; i < ( Page % 2 == 0 ? 24 : 23 ); ++i )
        {
            MouseController.ScrollMouse( -120 );
            std::this_thread::sleep_for( std::chrono::milliseconds( 50 + int( 10 * dis( gen ) ) ) );
        }

        EchoLeftToScan -= 3 * 6;
        // Data is at last row
        if ( EchoLeftToScan <= 3 * 6 ) break;

        if ( Page != 0 && Page % 16 == 0 )
        {
            MouseController.ScrollMouse( 120 );
            std::this_thread::sleep_for( std::chrono::milliseconds( 50 + int( 10 * dis( gen ) ) ) );
        }
    }

    if ( !Terminate && EchoLeftToScan != 0 )
    {
        const int  NumberOfRowLeft = std::ceil( EchoLeftToScan / 6.f );
        const auto StartingRow     = 4 - NumberOfRowLeft;

        ListOfCardIndex.clear( );
        for ( int j = StartingRow; j < 4; ++j )
            for ( int i = 0; i < 6 && EchoLeftToScan > 0; ++i, --EchoLeftToScan )
                ListOfCardIndex.emplace_back( i, j );
        ReadCardInLocations( ListOfCardIndex );
    }

    std::ofstream OutputJson( "echoes.yaml" );
    OutputJson << ResultYAMLEchos << std::endl;

    spdlog::info( "Scanning completed!" );

    system( "pause" );
}