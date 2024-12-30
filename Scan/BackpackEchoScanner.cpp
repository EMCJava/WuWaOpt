//
// Created by EMCJava on 12/30/2024.
//

#include "BackpackEchoScanner.hpp"

#include "Recognizer/EchoExtractor.hpp"
#include "Thread/AdaptiveJobPool.hpp"
#include "Win/GameHandle.hpp"

#include <Opt/OptUtil.hpp>

#include <opencv2/imgcodecs.hpp>

#include <spdlog/spdlog.h>

#include <thread>

std::random_device               RD;
std::mt19937                     RNG( RD( ) );
std::uniform_real_distribution<> CentralDis( 0.3, 0.7 );

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


void
BackpackEchoScanner::Initialize( )
{
    m_ShouldTerminate = false;
    if ( m_Initialized ) return;

    try
    {
        m_GameHandler = std::make_unique<GameHandle>( );
    }
    catch ( const ResolutionRuntimeError& e )
    {
        spdlog::error( std::vformat( m_LanguageProvider[ "WrongWinSize" ], std::make_format_args( e.ActualWidth, e.ActualHeight, e.DesiredWidth, e.DesiredHeight ) ) );
        system( "pause" );
        exit( 1 );
    }

    m_Initialized = true;
}

void
BackpackEchoScanner::MoveMouseToCard( FloatTy X, FloatTy Y )
{
    using namespace GameCoordinateConfigs;

    const MouseControl::MousePoint NextLocation =
        ( CardSize * static_cast<float>( CentralDis( RNG ) ) )
        + m_GameHandler->GetLeftTop( )
        + ( ScanLeftTop + CardSpacing * MouseControl::MousePoint { X, Y } );

    m_MouseController.MoveMouse( m_CurrentMousePos, NextLocation, m_ScanDelayMS + 20 * CentralDis( RNG ) );

    m_CurrentMousePos = NextLocation;
}

void
BackpackEchoScanner::SortBackpackEchoByLevel( )
{
    MouseControl::MousePoint ClickLocation { 435, 980 };
    ClickLocation += m_GameHandler->GetLeftTop( );

    m_MouseController.MoveMouse( m_CurrentMousePos, ClickLocation, 300 + 80 * ( CentralDis( RNG ) - 0.5 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
    m_MouseController.LeftClick( );

    m_CurrentMousePos = { 435, 610 };
    m_CurrentMousePos += m_GameHandler->GetLeftTop( );
    m_MouseController.MoveMouse( ClickLocation, m_CurrentMousePos, 300 + 80 * ( CentralDis( RNG ) - 0.5 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
    m_MouseController.LeftClick( 3 );
}

BackpackEchoScanner::BackpackEchoScanner( Loca& Localization )
    : m_MouseController( "data/MouseTrail.txt" )
    , m_LanguageProvider( Localization )
{
}

void
BackpackEchoScanner::SetScanDelay( int ScanDelayMS )
{
    m_ScanDelayMS = std::max( ScanDelayMS, 0 );
}

void
BackpackEchoScanner::Scan( int EchoCount, const std::function<bool( const FullStats& )>& CallBack )
{
    Initialize( );

    SortBackpackEchoByLevel( );

    MouseControl::MousePoint MouseLocation( 2 );

    std::vector<cv::Point> EchoCardCoords;
    for ( int j = 0; j < 3; ++j )
        for ( int i = 0; i < 6; ++i )
            EchoCardCoords.emplace_back( i, j );

    std::mutex CallBackLock;

    struct ScannerJobContext {
        cv::Mat ScreenCap;
    };

    AdaptiveJobPool ScannerJob( std::max( 1U, std::thread::hardware_concurrency( ) ) );
    ScannerJob.SetJobMaker( [ & ] {
        return [ &, Extractor = EchoExtractor { } ]( void* Context ) mutable {
            Stopwatch SW;

            const auto* TypedContext = static_cast<ScannerJobContext*>( Context );
            try
            {
                const auto FS = Extractor.ReadCard( TypedContext->ScreenCap );

                std::lock_guard CL( CallBackLock );
                if ( CallBack( FS ) )
                    spdlog::trace( "\n{}", Dump( YAML::Node( FS ) ) );
                else
                    m_ShouldTerminate = true;
            }
            catch ( const std::exception& e )
            {
                spdlog::error( "Error reading card: [{}] {}", e.what( ), m_LanguageProvider[ e.what( ) ] );

                std::stringstream FileName;
                FileName << std::chrono::high_resolution_clock::now( ).time_since_epoch( ).count( ) << "_defective.png";

                imwrite( FileName.str( ), TypedContext->ScreenCap );
            }

            delete TypedContext;
        };
    } );

    const auto StartCardReading = [ & ]( ) {
        auto* Context      = new ScannerJobContext;
        Context->ScreenCap = m_GameHandler->ScreenCap( );
        ScannerJob.NewJob( Context );
    };

    std::binary_semaphore CardReading { 1 };
    const auto            ReadCardAtLocations = [ & ]( auto&& Location ) {
        MoveMouseToCard( Location[ 0 ].x, Location[ 0 ].y );
        m_MouseController.LeftClick( 3 );

        for ( const auto& CardLocation : Location | std::views::drop( 1 ) )
        {
            // put this in job?
            std::thread { [ & ] {
                CardReading.acquire( );
                std::this_thread::sleep_for( std::chrono::milliseconds( m_ScanDelayMS ) );
                StartCardReading( );
                CardReading.release( );
            } }.detach( );

            MoveMouseToCard( CardLocation.x, CardLocation.y );

            CardReading.acquire( );
            if ( m_ShouldTerminate || GetAsyncKeyState( VK_SPACE ) )
            {
                m_ShouldTerminate = true;
                return;
            }
            m_MouseController.LeftClick( 3 );
            CardReading.release( );
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( m_ScanDelayMS ) );
        StartCardReading( );   // Read the final card
    };

    for ( int Page = 0;; ++Page )
    {
        spdlog::info( "Scanning at page {}...", Page );

        ReadCardAtLocations( EchoCardCoords );
        if ( m_ShouldTerminate ) break;

        for ( int i = 0; i < ( Page % 2 == 0 ? 24 : 23 ); ++i )
        {
            m_MouseController.ScrollMouse( -120 );
            std::this_thread::sleep_for( std::chrono::milliseconds( 40 + int( 10 * CentralDis( RNG ) ) ) );
        }

        EchoCount -= 3 * 6;
        // Data is at last row
        if ( EchoCount <= 3 * 6 ) break;

        if ( Page != 0 && Page % 16 == 0 )
        {
            m_MouseController.ScrollMouse( 120 );
            std::this_thread::sleep_for( std::chrono::milliseconds( 40 + int( 10 * CentralDis( RNG ) ) ) );
        }
    }

    if ( !m_ShouldTerminate && EchoCount != 0 )
    {
        const int  NumberOfRowLeft = std::ceil( EchoCount / 6.f );
        const auto StartingRow     = 4 - NumberOfRowLeft;

        EchoCardCoords.clear( );
        for ( int j = StartingRow; j < 4; ++j )
            for ( int i = 0; i < 6 && EchoCount > 0; ++i, --EchoCount )
                EchoCardCoords.emplace_back( i, j );
        ReadCardAtLocations( EchoCardCoords );
    }

    spdlog::info( "Worker created: {}", ScannerJob.GetWorkerCount( ) );
    ScannerJob.Clear( );
    spdlog::info( "Scanning completed!" );

    m_Initialized = false;
}