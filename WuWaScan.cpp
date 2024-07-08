//
// Created by EMCJava on 5/28/2024.
//
#include <windows.h>

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

const int RESIZED_IMAGE_WIDTH  = 20;
const int RESIZED_IMAGE_HEIGHT = 30;

std::random_device               rd;
std::mt19937                     gen( rd( ) );
std::uniform_real_distribution<> dis( 0.15, 0.85 );

std::vector<std::vector<std::pair<float, float>>> MouseTrails;

struct Template {
    char        name;
    std::string template_file;
    cv::Mat     template_mat;
    cv::Scalar  color;
    int         threshold;

    Template( char name, std::string template_file, cv::Scalar color, int threshold )
    {
        template_mat = imread( "data/" + template_file, cv::IMREAD_GRAYSCALE );

        this->name          = name;
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
    char     match;
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
}   // namespace Win
namespace ML
{
auto
TrainKNearest( auto&& Templates )
{
    // Training part
    cv::Mat matTrainingImagesAsFlattenedFloats;

    for ( int i = 0; i < Templates.size( ); i++ )
    {
        cv::Mat matROIResized;

        // resize image, this will be more consistent for recognition and storage
        cv::resize( Templates[ i ].template_mat, matROIResized, cv::Size( RESIZED_IMAGE_WIDTH, RESIZED_IMAGE_HEIGHT ) );

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
        matClassificationInts.at<int>( i, 0 ) = Templates[ i ].name;

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

    GetDIBits( GetDC( 0 ), hBitmap, 0, bmp.bmHeight, mat.data, (BITMAPINFO*) &bi, DIB_RGB_COLORS );

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

            while ( 1 )
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

                    MatchRects.emplace( pattern.name, MatchRect );
                } else
                    break;
            }
        }

        return MatchRects;
    }

    char KNNMatch( const cv::Mat& Src )
    {
        cv::Mat matCurrentChar( 0, 0, CV_32F );
        KNearestModel->findNearest( Src, 1, matCurrentChar );
        auto fltCurrentChar = matCurrentChar.at<float>( 0, 0 );
        return char( int( fltCurrentChar ) );
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
            Template {'1',          "1.png", cv::Scalar( 193, 182, 255 ), 220},
            Template {'2',          "2.png", cv::Scalar( 160, 158,  95 ), 210},
            Template {'3',          "3.png",  cv::Scalar( 80, 127, 255 ), 200},
            Template {'4',          "4.png", cv::Scalar( 169, 169, 169 ), 214},
            Template {'5',          "5.png", cv::Scalar( 107, 183, 189 ), 200},
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
            Template {eAutoAttackDamagePercentage,  "auto_attack.png", cv::Scalar( 255, 255,   0 ), 210},
            Template {     eHeavyAttackPercentage, "heavy_attack.png", cv::Scalar( 193, 182, 255 ), 200},
            Template {       eUltDamagePercentage,   "ult_damage.png", cv::Scalar( 193, 182, 255 ), 210},
            Template {     eSkillDamagePercentage,    "skill_dmg.png", cv::Scalar( 255, 240, 245 ), 210},
            Template {       eHealBonusPercentage,   "heal_bonus.png", cv::Scalar( 160, 158,  95 ), 210},
            Template {      eFireDamagePercentage,     "fire_dmg.png",  cv::Scalar( 80, 127, 255 ), 210},
            Template {       eAirDamagePercentage,      "air_dmg.png", cv::Scalar( 169, 169, 169 ), 210},
            Template {       eIceDamagePercentage,      "ice_dmg.png", cv::Scalar( 107, 183, 189 ), 210},
            Template {  eElectricDamagePercentage,     "elec_dmg.png", cv::Scalar( 204,  50, 153 ), 210},
            Template {      eDarkDamagePercentage,     "dark_dmg.png", cv::Scalar( 240, 255, 255 ), 210},
            Template {     eLightDamagePercentage,    "light_dmg.png", cv::Scalar( 255, 144,  30 ), 210},
            Template {                   eDefence,      "defence.png",  cv::Scalar( 47, 255, 173 ), 200},
            Template {                    eHealth,       "health.png",   cv::Scalar( 0, 255, 255 ), 210},
            Template {           eRegenPercentage,        "regen.png",   cv::Scalar( 0, 191, 255 ), 210},
            Template {                eCritDamage,     "crit_dmg.png", cv::Scalar( 153,  50, 204 ), 200},
            Template {                  eCritRate,    "crit_rate.png", cv::Scalar( 205, 133,  63 ), 210},
            Template {                        '1',       "cost_1.png", cv::Scalar( 193, 182, 255 ), 250},
            Template {                        '3',       "cost_3.png", cv::Scalar( 255, 240, 245 ), 250},
            Template {                        '4',       "cost_4.png", cv::Scalar( 160, 158,  95 ), 250},
    };
};

class Extractor
{
private:
    std::vector<char> MatchWithRecognizer( const cv::Mat& Src, auto& Recognizer )
    {
        cvtColor( Src, GrayImg, cv::COLOR_BGR2GRAY );
        cv::threshold( GrayImg, GrayImg, 155, 255, cv::THRESH_BINARY );
        cvtColor( GrayImg, MatchVisualizerImg, cv::COLOR_GRAY2BGR );

        auto& MatchRects = Recognizer.ExtractRect( GrayImg, MatchVisualizerImg );

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
                Result.push_back( CurrentMatch.match );
            } else
            {

                int left   = std::min_element( Overlaps.begin( ), Overlaps.end( ), []( auto&& a, auto&& b ) { return a.x < b.x; } )->x;
                int top    = std::min_element( Overlaps.begin( ), Overlaps.end( ), []( auto&& a, auto&& b ) { return a.y < b.y; } )->y;
                int right  = std::max_element( Overlaps.begin( ), Overlaps.end( ), []( auto&& a, auto&& b ) { return a.x + a.width < b.x + b.width; } )->br( ).x;
                int bottom = std::max_element( Overlaps.begin( ), Overlaps.end( ), []( auto&& a, auto&& b ) { return a.y + a.height < b.y + b.height; } )->br( ).y;

                cv::Mat matROI = GrayImg( cv::Rect { left, top, right - left, bottom - top } );

                cv::Mat matROIResized;
                cv::resize( matROI, matROIResized, cv::Size( RESIZED_IMAGE_WIDTH, RESIZED_IMAGE_HEIGHT ) );

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
    auto MatchText( const cv::Mat& Src )
    {
        return MatchWithRecognizer( Src, CRecognizer );
    }

    auto MatchType( const cv::Mat& Src )
    {
        return MatchWithRecognizer( Src, TRecognizer );
    }

    auto Match( const cv::Mat& Cimg, const cv::Mat& Timg )
    {
        auto CResult = MatchText( Cimg );
        auto TResult = MatchType( Timg );

        auto Types   = TResult | std::views::split( '\n' ) | std::views::transform( []( auto&& c ) -> char { return c.front( ); } );
        auto Numbers = CResult | std::views::split( '\n' )
            | std::views::transform( []( auto&& Number ) -> FloatTy {   // parse
                           FloatTy result;

                           if ( Number.back( ) == '%' )
                           {
                               int Convert = 0;
                               std::from_chars( Number.data( ), Number.data( ) + Number.size( ) - 1, Convert, 10 );
                               result = -Convert / 1000.f;
                           } else
                           {
                               int Convert = 0;
                               std::from_chars( Number.data( ), Number.data( ) + Number.size( ), Convert, 10 );
                               result = Convert;
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
            }
        }

        return Result;
    }

private:
    StatTypeRecognizer  TRecognizer;
    CharacterRecognizer CRecognizer;

    /*
     *
     * Cache per match
     *
     * */
    cv::Mat               GrayImg, MatchVisualizerImg;
    std::vector<cv::Rect> Overlaps;
};

void
AAA( )
{
    const int character_base_attack = 279;
    const int weapon_base_attack    = 263;
    const int base_attack           = character_base_attack + weapon_base_attack;

    const int     character_level   = 60;
    const int     enemy_level       = 54;
    const int     character_defence = 792 + character_level * 8;
    const int     enemy_defence     = 792 + enemy_level * 8;
    const FloatTy defence           = character_defence / FloatTy( character_defence + enemy_defence );

    const FloatTy enemy_resistance_percentage_sum = 0.1;

    // skill / attack
    const FloatTy skill_multiplier = 0.358;

    // team based
    const FloatTy damage_multiplier_percentage_sum = 0;

    EffectiveStats fixed_stats {
        .flat_attack       = 0,
        .percentage_attack = 0.018,
        .buff_multiplier   = 0,
        .crit_rate         = 0,
        .crit_damage       = 1,
    };

    // =========================================
    // Stats to optimize
    EffectiveStats              Weapon { .flat_attack = 0, .percentage_attack = 0 };
    std::vector<EffectiveStats> Echos {
        {.flat_attack = 150,             .percentage_attack = 0},
        { .flat_attack = 84,             .percentage_attack = 0},
        { .flat_attack = 68,             .percentage_attack = 0},
        {  .flat_attack = 0, .percentage_attack = 0.151 + 0.109},
        { .flat_attack = 50,         .percentage_attack = 0.122},
    };

    const auto Substat = std::reduce( Echos.begin( ), Echos.end( ), fixed_stats ) + Weapon;

    const FloatTy ActualDamage =
        Substat.NormalDamage( base_attack )
        * skill_multiplier
        * defence
        * ( 1 - enemy_resistance_percentage_sum )
        * ( 1 + damage_multiplier_percentage_sum );

    return;
}

void
MoveMouse( cv::Point Start, cv::Point End, int Millisecond = 1000 )
{
    const auto& MouseTrail = MouseTrails[ rand( ) % MouseTrails.size( ) ];

    const auto StartTime       = std::chrono::high_resolution_clock::now( );
    const auto MouseMoveOffset = End - Start;
    for ( int i = 0; i < MouseTrail.size( ) - 1; ++i )
    {
        const auto XCoordinate = std::round( Start.x + MouseMoveOffset.x * MouseTrail[ i ].first );
        const auto YCoordinate = std::round( Start.y + MouseMoveOffset.y * MouseTrail[ i ].second );

        SetCursorPos( XCoordinate, YCoordinate );

        auto EndTime = StartTime + std::chrono::microseconds( ( i == 0 ? 0 : i - 1 ) * Millisecond * 1000 / MouseTrail.size( ) );
        if ( i != 0 ) EndTime += std::chrono::microseconds( int( dis( gen ) * Millisecond * 1000 / MouseTrail.size( ) ) );
        std::this_thread::sleep_until( EndTime );
    }

    SetCursorPos( End.x, End.y );
}

void
LeftDown( )
{
    INPUT Input      = { 0 };
    Input.type       = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    ::SendInput( 1, &Input, sizeof( INPUT ) );
}

void
LeftUp( )
{
    INPUT Input      = { 0 };
    Input.type       = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    ::SendInput( 1, &Input, sizeof( INPUT ) );
}

void
LeftClick( bool DoubleClick = false, int Millisecond = -1 )
{
    LeftDown( );
    if ( Millisecond == -1 ) Millisecond = 120 + 40 * ( dis( gen ) - 0.5 );
    std::this_thread::sleep_for( std::chrono::microseconds( Millisecond ) );
    LeftUp( );

    if ( DoubleClick )
    {
        if ( Millisecond == -1 ) Millisecond = 120 + 40 * ( dis( gen ) - 0.5 );
        std::this_thread::sleep_for( std::chrono::microseconds( Millisecond ) );
        LeftClick( false, Millisecond );
    }
}


UINT
ScrollMouse( int scroll )
{
    INPUT input;
    POINT pos;
    GetCursorPos( &pos );

    input.type           = INPUT_MOUSE;
    input.mi.dwFlags     = MOUSEEVENTF_WHEEL;
    input.mi.time        = NULL;             // Windows will do the timestamp
    input.mi.mouseData   = (DWORD) scroll;   // A positive value indicates that the wheel was rotated forward, away from the user; a negative value indicates that the wheel was rotated backward, toward the user. One wheel click is defined as WHEEL_DELTA, which is 120.
    input.mi.dx          = pos.x;
    input.mi.dy          = pos.y;
    input.mi.dwExtraInfo = GetMessageExtraInfo( );

    return ::SendInput( 1, &input, sizeof( INPUT ) );
}

int
main( )
{
    {
        using namespace std::ranges;
        using std::views::chunk;
        using std::views::transform;

        std::ifstream MouseTrailInput( "data/MouseTrail.txt" );

        std::size_t TrailsCount, TrailResolution;
        MouseTrailInput >> TrailsCount >> TrailResolution;

        MouseTrails =
            istream_view<float>( MouseTrailInput )
            | chunk( TrailResolution * /* x and y coordinate */ 2 )
            | transform( []( auto&& r ) {
                  return r
                      | chunk( 2 )
                      | transform( []( auto&& r ) {
                             auto iter = r.begin( );
                             return std::make_pair( *r.begin( ), *++iter );
                         } )
                      | to<std::vector>( );
              } )
            | to<std::vector>( );

        for ( auto& Trail : MouseTrails )
            Trail.back( ) = { 1, 1 };
    }

    int EchoLeftToScan = 752;
    std::cout << "Scaning " << EchoLeftToScan << " echoes..." << std::endl;

    srand( time( nullptr ) );

    auto TargetWindowHandle = Win::GetGameWindow( );
    if ( TargetWindowHandle == nullptr ) return 0;

    HDC hScreenDC  = GetDC( GetDesktopWindow( ) );
    HDC hCaptureDC = CreateCompatibleDC( hScreenDC );   // create a device context to use yourself

    RECT WindowRect;
    GetWindowRect( TargetWindowHandle, &WindowRect );
    HBITMAP hCaptureBitmap = CreateCompatibleBitmap( hScreenDC, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top );   // create a bitmap

    // use the previously created device context with the bitmap
    SelectObject( hCaptureDC, hCaptureBitmap );

    static const float CardSpaceWidth  = 111;
    static const float CardSpaceHeight = 1230 / 9.f;

    static const int CardWidth  = 85;
    static const int CardHeight = 100;

    auto      ResultJson = json { };
    cv::Point CurrentLocation;
    Extractor extractor;

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

    const auto MouseToCard = [ & ]( int X, int Y ) {
        cv::Point NextLocation { WindowRect.left + 150
                                     + int( CardSpaceWidth * X + CardWidth * dis( gen ) ),
                                 WindowRect.top + 115
                                     + int( CardSpaceHeight * Y + CardHeight * dis( gen ) ) };
        MoveMouse( CurrentLocation, NextLocation, 200 + 80 * ( dis( gen ) - 0.5 ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );   // stop the momentu

        CurrentLocation = NextLocation;
    };

    const auto ReadCard = [ & ]( ) {
        std::cout << "=================Start Scan=================" << std::endl;
        cv::Mat InputImage = ScreenCap( );

        const cv::Rect FullRect { 885, 305, 350, 220 };
        const auto     FullImage = InputImage( FullRect );

        const cv::Rect StatRect { 0, 0, 26, 220 };
        const auto     StatImage = FullImage( StatRect );

        const cv::Rect NumberRect { 1150 - 885, 0, 80, 220 };
        const auto     NumberImage = FullImage( NumberRect );

        auto FS = extractor.Match( NumberImage, StatImage );

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

        //        cv::namedWindow( "InputImage", cv::WINDOW_AUTOSIZE );
        //        cv::imshow( "InputImage", InputImage );
        //        cv::setMouseCallback( "InputImage", []( int event, int x, int y, int flag, void* param ) {
        //                    cv::Mat& img =* reinterpret_cast<cv::Mat*>(param);
        //                    uchar b = img.data[img.channels()*(img.cols*y + x) + 0];
        //                    uchar g = img.data[img.channels()*(img.cols*y + x) + 1];
        //                    uchar r = img.data[img.channels()*(img.cols*y + x) + 2];
        //                    std::cout << "at (" << x << "," << y << ")" << std::endl;
        //                    cv::Mat colorImg(480, 640, CV_8UC3, cv::Scalar(b, g, r));
        //                    std::cout << (int)r << " " << (int)g << " " << (int)b << std::endl;
        //                    cv::imshow("Color", colorImg); }, reinterpret_cast<void*>( &InputImage ) );
        //        cv::waitKey( 0 );

        //        cv::namedWindow( "InputImage", cv::WINDOW_AUTOSIZE );
        //        cv::imshow( "InputImage", InputImage );
        //        cv::setMouseCallback( "InputImage", []( int event, int x, int y, int flag, void* param ) {
        //                    cv::Mat& img =* reinterpret_cast<cv::Mat*>(param);
        //                    uchar b = img.data[img.channels()*(img.cols*y + x) + 0];
        //                    uchar g = img.data[img.channels()*(img.cols*y + x) + 1];
        //                    uchar r = img.data[img.channels()*(img.cols*y + x) + 2];
        //                    std::cout << "at (" << x << "," << y << ")" << std::endl;
        //                    cv::Mat colorImg(480, 640, CV_8UC3, cv::Scalar(b, g, r));
        //                    std::cout << (int)r << " " << (int)g << " " << (int)b << std::endl;
        //                    cv::imshow("Color", colorImg); }, reinterpret_cast<void*>( &InputImage ) );
        //        cv::waitKey( 0 );

        int SetColorCoordinateX = 977, SetColorCoordinateY = 242;
        FS.Set = MatchColorToSet( { InputImage.data[ InputImage.channels( ) * ( InputImage.cols * SetColorCoordinateY + SetColorCoordinateX ) + 2 ],
                                    InputImage.data[ InputImage.channels( ) * ( InputImage.cols * SetColorCoordinateY + SetColorCoordinateX ) + 1 ],
                                    InputImage.data[ InputImage.channels( ) * ( InputImage.cols * SetColorCoordinateY + SetColorCoordinateX ) + 0 ] } );

        const cv::Rect LevelRect { 1202, 195, 28, 22 };
        const auto     LevelImage = InputImage( LevelRect );
        const auto     LevelChars = extractor.MatchText( LevelImage );
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
        LeftClick( true );

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
            LeftClick( true );
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
            ScrollMouse( -120 );
            std::this_thread::sleep_for( std::chrono::milliseconds( 50 + int( 10 * dis( gen ) ) ) );
        }

        EchoLeftToScan -= 3 * 6;
        // Data is at last row
        if ( EchoLeftToScan <= 3 * 6 ) break;

        if ( Page != 0 && Page % 16 == 0 )
        {
            ScrollMouse( 120 );
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