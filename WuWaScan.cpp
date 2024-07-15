//
// Created by EMCJava on 5/28/2024.
//
#include <windows.h>
#include <Psapi.h>

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
#include "Scan/MouseControl.hpp"

const int RESIZED_IMAGE_WIDTH  = 1200;
const int RESIZED_IMAGE_HEIGHT = 300;

std::random_device               rd;
std::mt19937                     gen( rd( ) );
std::uniform_real_distribution<> dis( 0.15, 0.85 );

struct Template {
    int32_t     template_id;
    std::string template_file;
    cv::Mat     template_mat;
    cv::Scalar  color;
    int         threshold;

    Template( int32_t name, std::string template_file, cv::Scalar color, int threshold )
    {
        if ( !std::filesystem::exists( template_file ) ) template_file = "data/" + template_file;
        template_mat = cv::imread( template_file, cv::IMREAD_GRAYSCALE );

        this->template_id   = name;
        this->template_file = std::move( template_file );
        this->color         = std::move( color );
        this->threshold     = threshold;

        if ( template_mat.empty( ) )
        {
            std::cout << "Error reading template file: " << template_file << std::endl;
            return;
        }
    }
};

struct Record {
    int      match;
    cv::Rect rect;
};
class CompareRecord
{
public:
    bool operator( )( Record a, Record b ) const
    {
        if ( a.rect.y + 10 < b.rect.y ) return false;
        if ( a.rect.y > b.rect.y + 10 ) return true;
        return a.rect.x > b.rect.x;
    }
};

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

    std::array<char, 256> window_title { 0 };
    HWND                  Foreground = nullptr;
    while ( iotaFuture.wait_for( std::chrono::milliseconds( 100 ) ) != std::future_status::ready )
    {
        auto FG = GetForegroundWindow( );
        if ( FG == MessageHWND ) continue;
        if ( ( Foreground = FG ) )
        {
            GetWindowText( Foreground, window_title.data( ), window_title.size( ) );
            SetWindowText( MessageCaptionHWND, window_title.data( ) );
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
                std::cout << "Full path of the executable: " << executablePath << std::endl;
                *(HWND*) lParam = hwnd;
                return FALSE;
            }
        }

        CloseHandle( hProc );
    }

    return TRUE;
}
}   // namespace Win
namespace ML
{
void
PixelPerfectResize( const cv::Mat& src, cv::Mat& dst, int width, int height )
{
    // Create a destination matrix with the desired size
    dst = cv::Mat::zeros( height, width, src.type( ) );

    // Calculate the scale factors
    float scaleX = (float) src.cols / (float) width;
    float scaleY = (float) src.rows / (float) height;

    // Loop over the destination image
    for ( int y = 0; y < height; ++y )
    {
        for ( int x = 0; x < width; ++x )
        {
            // Find the corresponding position in the source image
            int srcX = std::min( (int) ( (float) x * scaleX ), src.cols - 1 );
            int srcY = std::min( (int) ( (float) y * scaleY ), src.rows - 1 );

            // Assign the pixel value
            dst.at<uint8_t>( y, x ) = src.at<uint8_t>( srcY, srcX );
        }
    }
}


auto
TrainKNearest( auto&& Templates )
{
    // Training part
    cv::Mat matTrainingImagesAsFlattenedFloats;

    for ( int i = 0; i < Templates.size( ); i++ )
    {
        cv::Mat matROIResized;

        // resize image, this will be more consistent for recognition and storage
        PixelPerfectResize( Templates[ i ].template_mat, matROIResized, RESIZED_IMAGE_WIDTH, RESIZED_IMAGE_HEIGHT );

        // now add the training image (some conversion is necessary first) . . .
        cv::Mat matImageFloat;
        // convert Mat to float
        matROIResized.convertTo( matImageFloat, CV_32FC1 );

        // flatten
        cv::Mat matImageFlattenedFloat = matImageFloat.reshape( 1, 1 );

        matTrainingImagesAsFlattenedFloats.push_back( matImageFlattenedFloat );
    }

    cv::Mat matClassificationInts( Templates.size( ), 1, CV_32S );
    for ( int i = 0; i < Templates.size( ); i++ )
        matClassificationInts.at<int>( i, 0 ) = Templates[ i ].template_id;

    auto model = cv::ml::KNearest::create( );
    model->train( matTrainingImagesAsFlattenedFloats, cv::ml::ROW_SAMPLE, matClassificationInts );

    return model;
}
}   // namespace ML
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

template <class T, class S, class C>
S&
Container( std::priority_queue<T, S, C>& q )
{
    struct HackedQueue : private std::priority_queue<T, S, C> {
        static S& Container( std::priority_queue<T, S, C>& q )
        {
            return q.*&HackedQueue::c;
        }
    };
    return HackedQueue::Container( q );
}

template <typename Child>
class RecognizerBase
{
public:
    void Initialize( )
    {
        KNearestModel = ML::TrainKNearest( ( (Child*) this )->GetTemplates( ) );
    }

    auto& ExtractRect( const cv::Mat& Src, const cv::Mat& MatchVisualizer )
    {
        Container( MatchRects ).clear( );
        for ( const auto& [ index, pattern ] : std::ranges::enumerate_view( ( (Child*) this )->GetTemplates( ) ) )
        {
            if ( Src.rows - pattern.template_mat.rows + 1 <= 0 || Src.cols - pattern.template_mat.cols + 1 <= 0 ) continue;
            cv::Mat res_32f( Src.rows - pattern.template_mat.rows + 1, Src.cols - pattern.template_mat.cols + 1, CV_32FC1 );
            matchTemplate( Src, pattern.template_mat, res_32f, cv::TM_CCORR_NORMED );

            res_32f.convertTo( MatchResult, CV_8U, 255.0 );
            cv::threshold( MatchResult, MatchResult, pattern.threshold, 255, cv::THRESH_BINARY );

            while ( true )
            {
                double    minval, maxval;
                cv::Point minloc, maxloc;
                minMaxLoc( MatchResult, &minval, &maxval, &minloc, &maxloc );

                if ( maxval > 0 )
                {
                    cv::Rect MatchRect { maxloc,
                                         cv::Point( maxloc.x + pattern.template_mat.cols,
                                                    maxloc.y + pattern.template_mat.rows ) };

                    rectangle( MatchVisualizer, MatchRect, pattern.color, 2 );
                    floodFill( MatchResult, maxloc, 0 );

                    MatchRects.emplace( pattern.template_id, MatchRect );
                } else
                    break;
            }
        }

        return MatchRects;
    }

    int KNNMatch( const cv::Mat& Src )
    {
        cv::Mat matCurrentChar( 0, 0, CV_32F );
        KNearestModel->findNearest( Src, 1, matCurrentChar );
        return (int) std::round( matCurrentChar.at<float>( 0, 0 ) );
    }

private:
    cv::Ptr<cv::ml::KNearest> KNearestModel;
    cv::Mat                   MatchResult;

    /*
     *
     * Cache per match
     *
     * */
    std::priority_queue<Record, std::vector<Record>, CompareRecord> MatchRects;
};

class CharacterRecognizer : public RecognizerBase<CharacterRecognizer>
{
public:
    CharacterRecognizer( )
    {
        Initialize( );
    }

    [[nodiscard]] const auto& GetTemplates( ) const
    {
        return Templates;
    }

private:
    std::array<Template, 11>
        Templates {
            Template {'%', "percentage.png", cv::Scalar( 255, 248, 240 ), 190},
            Template {'0',          "0.png", cv::Scalar( 255, 255,   0 ), 205},
            Template {'1',          "1.png", cv::Scalar( 193, 182, 255 ), 240},
            Template {'2',          "2.png", cv::Scalar( 160, 158,  95 ), 210},
            Template {'3',          "3.png",  cv::Scalar( 80, 127, 255 ), 200},
            Template {'4',          "4.png", cv::Scalar( 169, 169, 169 ), 214},
            Template {'5',          "5.png", cv::Scalar( 107, 183, 189 ), 190},
            Template {'6',          "6.png", cv::Scalar( 204,  50, 153 ), 205},
            Template {'7',          "7.png", cv::Scalar( 240, 255, 255 ), 220},
            Template {'8',          "8.png", cv::Scalar( 255, 144,  30 ), 205},
            Template {'9',          "9.png",  cv::Scalar( 47, 255, 173 ), 205},
    };
};

class StatTypeRecognizer : public RecognizerBase<StatTypeRecognizer>
{
public:
    StatTypeRecognizer( )
    {
        Initialize( );
    }

    [[nodiscard]] const auto& GetTemplates( ) const
    {
        return Templates;
    }

private:
    std::vector<Template>
        Templates {
            Template {                    eAttack,       "attack.png", cv::Scalar( 255, 248, 240 ), 210},
            Template {eAutoAttackDamagePercentage,  "auto_attack.png", cv::Scalar( 255, 255,   0 ), 200},
            Template {     eHeavyAttackPercentage, "heavy_attack.png", cv::Scalar( 193, 182, 255 ), 200},
            Template {       eUltDamagePercentage,   "ult_damage.png", cv::Scalar( 193, 182, 255 ), 180},
            Template {     eSkillDamagePercentage,    "skill_dmg.png", cv::Scalar( 255,   0, 245 ), 200},
            Template {       eHealBonusPercentage,   "heal_bonus.png", cv::Scalar( 160, 158,  95 ), 210},
            Template {      eFireDamagePercentage,     "fire_dmg.png",  cv::Scalar( 80, 127, 255 ), 210},
            Template {       eAirDamagePercentage,      "air_dmg.png", cv::Scalar( 169, 169, 169 ), 210},
            Template {       eIceDamagePercentage,      "ice_dmg.png", cv::Scalar( 107, 183, 189 ), 210},
            Template {  eElectricDamagePercentage,     "elec_dmg.png", cv::Scalar( 204,  50, 153 ), 210},
            Template {      eDarkDamagePercentage,     "dark_dmg.png", cv::Scalar( 240, 255, 255 ), 210},
            Template {     eLightDamagePercentage,    "light_dmg.png", cv::Scalar( 255, 144,  30 ), 210},
            Template {                   eDefence,      "defence.png",  cv::Scalar( 47, 255, 173 ), 180},
            Template {                    eHealth,       "health.png",   cv::Scalar( 0, 255, 255 ), 210},
            Template {           eRegenPercentage,        "regen.png",   cv::Scalar( 0, 191, 255 ), 200},
            Template {                eCritDamage,     "crit_dmg.png", cv::Scalar( 153,  50, 204 ), 200},
            Template {                  eCritRate,    "crit_rate.png", cv::Scalar( 205, 133,  63 ), 210},
            Template {                        '1',       "cost_1.png", cv::Scalar( 193, 182, 255 ), 250},
            Template {                        '3',       "cost_3.png", cv::Scalar( 255, 240, 245 ), 250},
            Template {                        '4',       "cost_4.png", cv::Scalar( 160, 158,  95 ), 250},
    };
};

class EchoNameRecognizer : public RecognizerBase<EchoNameRecognizer>
{
public:
    explicit EchoNameRecognizer( const std::filesystem::path& EchoNameFolder = "data/names" )
    {
        Templates = std::filesystem::directory_iterator { EchoNameFolder }
            | std::views::enumerate
            | std::views::transform( []( const auto& Index_DirEntry ) {
                        const auto& path = std::get<1>( Index_DirEntry ).path( );
                        std::cout << "Loading template: " << path.stem( ) << std::endl;
                        return Template {
                            (int32_t) std::get<0>( Index_DirEntry ),
                            std::filesystem::absolute( path ).string( ),
                            cv::Scalar(
                                100 + rand( ) % ( 255 - 100 ),
                                100 + rand( ) % ( 255 - 100 ),
                                100 + rand( ) % ( 255 - 100 ) ),
                            220 };
                    } )
            | std::ranges::to<std::vector>( );

        Initialize( );
    }

    [[nodiscard]] const auto& GetTemplates( ) const
    {
        return Templates;
    }

private:
    std::vector<Template> Templates;
};

class Extractor
{
private:
    void ThresholdPreProcessor( const cv::Mat& Src )
    {
        cvtColor( Src, GrayImg, cv::COLOR_BGR2GRAY );
        cv::threshold( GrayImg, GrayImg, 153, 255, cv::THRESH_BINARY );
        cvtColor( GrayImg, MatchVisualizerImg, cv::COLOR_GRAY2BGR );
    }

    void InYellowRangePreProcessor( const cv::Mat& Src )
    {
        cv::Mat SrcHSV;
        cvtColor( Src, SrcHSV, cv::COLOR_BGR2HSV );
        cv::inRange( SrcHSV, cv::Scalar( 25, 0, 150 ), cv::Scalar( 35, 255, 255 ), GrayImg );
        cvtColor( GrayImg, MatchVisualizerImg, cv::COLOR_GRAY2BGR );
    }

    std::vector<char> MatchWithRecognizer( const cv::Mat&     Src,
                                           auto&              Recognizer,
                                           const std::string& MatchName                        = "",
                                           void ( Extractor::*PreProcessor )( const cv::Mat& ) = &Extractor::ThresholdPreProcessor )
    {

        ( this->*PreProcessor )( Src );

        auto& MatchRects = Recognizer.ExtractRect( GrayImg, MatchVisualizerImg );

        if ( !MatchName.empty( ) )
        {
            const int MinimumWidth  = 300;
            const int MinimumHeight = 300;
            const int FinalWidth    = std::max( MatchVisualizerImg.cols + 100, MinimumWidth );

            cv::Mat newImage = cv::Mat::zeros( std::max( MatchVisualizerImg.rows, MinimumHeight ), FinalWidth, MatchVisualizerImg.type( ) );
            MatchVisualizerImg.copyTo( newImage( cv::Rect( 0, 0, MatchVisualizerImg.cols, MatchVisualizerImg.rows ) ) );

            cv::namedWindow( MatchName, cv::WINDOW_NORMAL );
            cv::resizeWindow( MatchName, newImage.cols, newImage.rows );
            cv::imshow( MatchName, newImage );
        }

        std::vector<char> Result;

        while ( !MatchRects.empty( ) )
        {
            const auto CurrentMatch = MatchRects.top( );
            MatchRects.pop( );

            // Check for overlap
            Overlaps.assign( { CurrentMatch.rect } );
            while ( !MatchRects.empty( ) )
            {
                const auto& NextMatch = MatchRects.top( );
                if ( NextMatch.rect.y <= CurrentMatch.rect.y + 10
                     && NextMatch.rect.x <= CurrentMatch.rect.x + 6 )
                {
                    Overlaps.emplace_back( NextMatch.rect );
                    MatchRects.pop( );
                    continue;
                }
                break;
            }

            if ( Overlaps.size( ) == 1 )
            {
                // std::cout << CurrentMatch.match;
                Result.push_back( (char) CurrentMatch.match );
            } else
            {

                int left   = std::min_element( Overlaps.begin( ), Overlaps.end( ), []( auto&& a, auto&& b ) { return a.x < b.x; } )->x;
                int top    = std::min_element( Overlaps.begin( ), Overlaps.end( ), []( auto&& a, auto&& b ) { return a.y < b.y; } )->y;
                int right  = std::max_element( Overlaps.begin( ), Overlaps.end( ), []( auto&& a, auto&& b ) { return a.x + a.width < b.x + b.width; } )->br( ).x;
                int bottom = std::max_element( Overlaps.begin( ), Overlaps.end( ), []( auto&& a, auto&& b ) { return a.y + a.height < b.y + b.height; } )->br( ).y;

                cv::Mat matROI = GrayImg( cv::Rect { left, top, right - left, bottom - top } );

                cv::Mat matROIResized;
                ML::PixelPerfectResize( matROI, matROIResized, RESIZED_IMAGE_WIDTH, RESIZED_IMAGE_HEIGHT );

                cv::Mat matROIFloat;
                matROIResized.convertTo( matROIFloat, CV_32FC1 );

                cv::Mat matROIFlattenedFloat = matROIFloat.reshape( 1, 1 );

                cv::Mat matCurrentChar( 0, 0, CV_32F );

                // std::cout << '[' << Recognizer.KNNMatch( matROIFlattenedFloat ) << ']';
                Result.push_back( Recognizer.KNNMatch( matROIFlattenedFloat ) );
            }

            if ( !MatchRects.empty( )
                 && MatchRects.top( ).rect.y > CurrentMatch.rect.y + 10 )
                Result.push_back( '\n' );
        }

        return Result;
    }

public:
    auto MatchText( const cv::Mat& Src, const std::string& MatchName = "" )
    {
        return MatchWithRecognizer( Src, CRecognizer, MatchName );
    }

    auto MatchType( const cv::Mat& Src, const std::string& MatchName = "" )
    {
        return MatchWithRecognizer( Src, TRecognizer, MatchName );
    }

    auto MatchEchoName( const cv::Mat& Src, const std::string& MatchName = "" )
    {
        return MatchWithRecognizer( Src, NRecognizer, MatchName, &Extractor::InYellowRangePreProcessor );
    }

    auto Match( const cv::Mat& Cimg, const cv::Mat& Timg, const cv::Mat& Nimg )
    {
        auto CResult = MatchText( Cimg, "Stat Number" );
        auto TResult = MatchType( Timg, "Stat Type" );

        auto NResult = MatchEchoName( Nimg, "Echo Name" );

        auto Types   = TResult | std::views::split( '\n' ) | std::views::transform( []( auto&& c ) -> char { return c.front( ); } );
        auto Numbers = CResult | std::views::split( '\n' )
            | std::views::transform( []( auto&& Number ) -> FloatTy {   // parse
                           FloatTy result;

                           if ( Number.back( ) == '%' )
                           {
                               int Convert = 0;
                               std::from_chars( Number.data( ), Number.data( ) + Number.size( ) - 1, Convert, 10 );
                               result = -(FloatTy) Convert / (FloatTy) 1000;
                           } else
                           {
                               int Convert = 0;
                               std::from_chars( Number.data( ), Number.data( ) + Number.size( ), Convert, 10 );
                               result = (FloatTy) Convert;
                           }

                           return result;
                       } );

        FullStats Result;
        for ( auto [ Type, Number ] : std::views::zip( Types, Numbers ) )
        {
            // Percentage number
            if ( Number < 0 )
            {
                if ( Type == eAttack ) Type = eAttackPercentage;
                if ( Type == eHealth ) Type = eHealthPercentage;
                if ( Type == eDefence ) Type = eDefencePercentage;
                Number = -Number;
            }

            switch ( Type )
            {
#define SAVE( name ) \
    case e##name: Result.name += Number; break;
                SAVE( Attack )
                SAVE( AttackPercentage )
                SAVE( AutoAttackDamagePercentage )
                SAVE( HeavyAttackPercentage )
                SAVE( UltDamagePercentage )
                SAVE( SkillDamagePercentage )
                SAVE( HealBonusPercentage )
                SAVE( FireDamagePercentage )
                SAVE( AirDamagePercentage )
                SAVE( IceDamagePercentage )
                SAVE( ElectricDamagePercentage )
                SAVE( DarkDamagePercentage )
                SAVE( LightDamagePercentage )
                SAVE( Defence )
                SAVE( DefencePercentage )
                SAVE( Health )
                SAVE( HealthPercentage )
                SAVE( RegenPercentage )
                SAVE( CritDamage )
                SAVE( CritRate )
#undef SAVE
            default:
                std::unreachable( );
            }
        }

        if ( !NResult.empty( ) )
        {
            std::filesystem::path TemplateName = NRecognizer.GetTemplates( )[ NResult.front( ) ].template_file;
            Result.EchoName                    = TemplateName.stem( ).string( );
        }

        return Result;
    }

private:
    StatTypeRecognizer  TRecognizer;
    CharacterRecognizer CRecognizer;
    EchoNameRecognizer  NRecognizer;

    /*
     *
     * Cache per match
     *
     * */
    cv::Mat               GrayImg, MatchVisualizerImg;
    std::vector<cv::Rect> Overlaps;
};

int
main( )
{
    HWND TargetWindowHandle = nullptr;
    EnumWindows( Win::EnumGameProc, reinterpret_cast<LPARAM>( &TargetWindowHandle ) );
    if ( TargetWindowHandle == nullptr )
    {
        TargetWindowHandle = Win::GetGameWindow( );
        if ( TargetWindowHandle == nullptr ) return 0;
    }

    std::wstring TargetWindowTitle( 256, L'\0' );
    GetWindowTextW( TargetWindowHandle, TargetWindowTitle.data( ), (int) TargetWindowTitle.size( ) );
    setlocale( LC_CTYPE, "en_US.UTF-8" );

    std::wcout << TargetWindowHandle << ": " << TargetWindowTitle.data( ) << std::endl;
    SetForegroundWindow( TargetWindowHandle );

    HDC hScreenDC  = GetDC( GetDesktopWindow( ) );
    HDC hCaptureDC = CreateCompatibleDC( hScreenDC );   // create a device context to use yourself

    RECT WindowRect;
    GetWindowRect( TargetWindowHandle, &WindowRect );
    HBITMAP hCaptureBitmap = CreateCompatibleBitmap( hScreenDC, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top );   // create a bitmap

    // use the previously created device context with the bitmap
    SelectObject( hCaptureDC, hCaptureBitmap );

    int EchoLeftToScan = 848;
    std::cout << "Scaning " << EchoLeftToScan << " echoes..." << std::endl;

    srand( time( nullptr ) );

    static const float CardSpaceWidth  = 111;
    static const float CardSpaceHeight = 1230 / 9.f;

    static const int CardWidth  = 85;
    static const int CardHeight = 100;

    auto                     ResultJson = json { };
    MouseControl::MousePoint MouseLocation( 2 );
    Extractor                extractor;

    std::vector<cv::Point> ListOfCardIndex;
    for ( int j = 0; j < 3; ++j )
        for ( int i = 0; i < 6; ++i )
            ListOfCardIndex.emplace_back( i, j );

    auto& ResultJsonEchos = ResultJson[ "echos" ];

    std::binary_semaphore CardReading { 1 };

    const auto ScreenCap = [ & ]( ) {
        // GetWindowRect( TargetWindowHandle, &WindowRect );

        BitBlt( hCaptureDC,
                0, 0, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top,
                hScreenDC,
                WindowRect.left, WindowRect.top, SRCCOPY );
        return CVUtils::HBitmapToMat( hCaptureBitmap );
    };

    MouseControl MouseController( "data/MouseTrail.txt" );
    const auto   MouseToCard = [ & ]( int X, int Y ) {
        MouseControl::MousePoint NextLocation { WindowRect.left + 150
                                                    + CardSpaceWidth * X + CardWidth * (float) dis( gen ),
                                                WindowRect.top + 115
                                                    + CardSpaceHeight * Y + CardHeight * (float) dis( gen ) };
        MouseController.MoveMouse( MouseLocation, NextLocation, 200 + 80 * ( dis( gen ) - 0.5 ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

        MouseLocation = NextLocation;
    };

    const auto ReadCard = [ & ]( ) {
        std::cout << "=================Start Scan=================" << std::endl;
        cv::Mat InputImage = ScreenCap( );

        const cv::Rect FullRect { 885, 305, 350, 220 };
        const auto     FullImage = InputImage( FullRect );

        const cv::Rect NameRect { 880, 115, 200, 25 };
        const auto     NameImage = InputImage( NameRect );

        const cv::Rect StatRect { 0, 0, 26, 220 };
        const auto     StatImage = FullImage( StatRect );

        const cv::Rect NumberRect { 1150 - 885, 0, 80, 220 };
        const auto     NumberImage = FullImage( NumberRect );

        auto FS = extractor.Match( NumberImage, StatImage, NameImage );

        const cv::Rect CostRect { 1148, 170, 76, 18 };
        const auto     CostImage = InputImage( CostRect ).clone( );
        cv::Mat        CostThresholeImage;
        cvtColor( CostImage, CostThresholeImage, cv::COLOR_BGR2GRAY );
        cv::adaptiveThreshold( CostThresholeImage, CostThresholeImage, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 17, -10 );
        cvtColor( CostThresholeImage, CostThresholeImage, cv::COLOR_GRAY2BGR );
        const auto CostChars = extractor.MatchType( CostThresholeImage );
        if ( CostChars.empty( ) || CostChars[ 0 ] < '1' || CostChars[ 0 ] > '4' )
        {
            std::cout << "Cost is invalid: " << ( CostChars.empty( ) ? "null" : std::string( 1, CostChars[ 0 ] ) ) << " (" << CostChars.size( ) << ")" << std::endl;
            return;
        }
        FS.Cost = CostChars[ 0 ] - '0';

        int SetColorCoordinateX = 977, SetColorCoordinateY = 242;
        FS.Set = MatchColorToSet( { InputImage.data[ InputImage.channels( ) * ( InputImage.cols * SetColorCoordinateY + SetColorCoordinateX ) + 2 ],
                                    InputImage.data[ InputImage.channels( ) * ( InputImage.cols * SetColorCoordinateY + SetColorCoordinateX ) + 1 ],
                                    InputImage.data[ InputImage.channels( ) * ( InputImage.cols * SetColorCoordinateY + SetColorCoordinateX ) + 0 ] } );

        const cv::Rect LevelRect { 1202, 195, 28, 22 };
        const auto     LevelImage = InputImage( LevelRect );
        const auto     LevelChars = extractor.MatchText( LevelImage, "Level" );
        std::from_chars( LevelChars.data( ), LevelChars.data( ) + LevelChars.size( ), FS.Level, 10 );
        if ( FS.Level > 25 )
        {
            std::cout << "Level is invalid: " << FS.Level << std::endl;
            return;
        }

        std::cout << json( FS ) << std::endl;
        ResultJsonEchos.push_back( FS );
    };

    const auto ReadCardInLocations = [ & ]( auto&& Location ) {
        MouseToCard( Location[ 0 ].x, Location[ 0 ].y );
        MouseController.LeftClick( true );

        for ( const auto& CardLocation : Location | std::views::drop( 1 ) )
        {
            std::thread { [ &CardReading, &ReadCard ] {
                CardReading.acquire( );
                std::this_thread::sleep_for( std::chrono::milliseconds( 120 ) );
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

    DeleteObject( hCaptureBitmap );
    DeleteDC( hCaptureDC );
}