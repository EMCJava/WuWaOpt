//
// Created by EMCJava on 5/28/2024.
//

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/ml/ml.hpp>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <chrono>
#include <fstream>
#include <future>
#include <numeric>
#include <queue>
#include <random>
#include <ranges>
#include <utility>
#include <vector>
#include <semaphore>

#include "Opt/FullStats.hpp"
#include "Scan/Win/MouseControl.hpp"
#include "Scan/Win/GameHandle.hpp"
#include "Scan/Recognizer/EchoExtractor.hpp"

#include <spdlog/spdlog.h>

std::random_device               rd;
std::mt19937                     gen( rd( ) );
std::uniform_real_distribution<> dis( 0.15, 0.85 );

int
main( )
{
    srand( time( nullptr ) );

    GameHandle GameHandler;

    int EchoLeftToScan = 848;
    std::cout << "Scaning " << EchoLeftToScan << " echoes..." << std::endl;

    MouseControl::MousePoint MouseLocation( 2 );

    json  ResultJson { };
    auto& ResultJsonEchos = ResultJson[ "echos" ];

    std::vector<cv::Point> ListOfCardIndex;
    for ( int j = 0; j < 3; ++j )
        for ( int i = 0; i < 6; ++i )
            ListOfCardIndex.emplace_back( i, j );

    MouseControl MouseController( "data/MouseTrail.txt" );
    const auto   MouseToCard = [ & ]( int X, int Y ) {
        static const float CardSpaceWidth  = 111;
        static const float CardSpaceHeight = 1230 / 9.f;

        static const int CardWidth  = 85;
        static const int CardHeight = 100;

        MouseControl::MousePoint NextLocation = { 150 + CardSpaceWidth * X + CardWidth * (float) dis( gen ),
                                                  115 + CardSpaceHeight * Y + CardHeight * (float) dis( gen ) };
        NextLocation += GameHandler.GetLeftTop( );

        MouseController.MoveMouse( MouseLocation, NextLocation, 300 + 80 * ( dis( gen ) - 0.5 ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

        MouseLocation = NextLocation;
    };

    EchoExtractor Extractor;
    const auto    ReadCard = [ & ]( ) {
        std::cout << "=================Start Scan=================" << std::endl;
        Stopwatch  SW;
        const auto FS = Extractor.ReadCard( GameHandler.ScreenCap( ) );
        std::cout << json( FS ) << std::endl;
        ResultJsonEchos.push_back( FS );
    };

//    while ( true )
//    {
//        {
//            Stopwatch  SW;
//            const auto FS = Extractor.ReadCard( GameHandler.ScreenCap( ) );
//        }
//        cv::waitKey( 1 );
//    }

    std::binary_semaphore CardReading { 1 };
    const auto            ReadCardInLocations = [ & ]( auto&& Location ) {
        MouseToCard( Location[ 0 ].x, Location[ 0 ].y );
        MouseController.LeftClick( true );

        for ( const auto& CardLocation : Location | std::views::drop( 1 ) )
        {
            std::thread { [ &CardReading, &ReadCard ] {
                CardReading.acquire( );
                std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
                ReadCard( );
                CardReading.release( );
            } }.detach( );

            MouseToCard( CardLocation.x, CardLocation.y );

            CardReading.acquire( );
            MouseController.LeftClick( true );
            CardReading.release( );
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 120 ) );
        ReadCard( );   // Read the final card
    };

    for ( int Page = 0;; ++Page )
    {
        std::cout << "Page: " << Page << std::endl;

        ReadCardInLocations( ListOfCardIndex );

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

    if ( EchoLeftToScan != 0 )
    {
        const int  NumberOfRowLeft = std::ceil( EchoLeftToScan / 6.f );
        const auto StartingRow     = 4 - NumberOfRowLeft;

        ListOfCardIndex.clear( );
        for ( int j = StartingRow; j < 4; ++j )
            for ( int i = 0; i < 6 && EchoLeftToScan > 0; ++i, --EchoLeftToScan )
                ListOfCardIndex.emplace_back( i, j );
        ReadCardInLocations( ListOfCardIndex );
    }

    std::ofstream OutputJson( "data/echos.json" );
    OutputJson << ResultJson << std::endl;

    std::cout << ResultJsonEchos << std::endl;
    std::cin.get( );
}