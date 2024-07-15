//
// Created by EMCJava on 7/15/2024.
//

#include "EchoExtractor.hpp"

void
EchoExtractor::ThresholdPreProcessor( const cv::Mat& Src )
{
    cvtColor( Src, m_GrayImg, cv::COLOR_BGR2GRAY );
    cv::threshold( m_GrayImg, m_GrayImg, 153, 255, cv::THRESH_BINARY );
    cvtColor( m_GrayImg, m_MatchVisualizerImg, cv::COLOR_GRAY2BGR );
}

void
EchoExtractor::CostThresholdPreProcessor( const cv::Mat& Src )
{
    cvtColor( Src, m_GrayImg, cv::COLOR_BGR2GRAY );
    cv::adaptiveThreshold( m_GrayImg, m_GrayImg, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 17, -10 );
    cvtColor( m_GrayImg, m_MatchVisualizerImg, cv::COLOR_GRAY2BGR );
}


void
EchoExtractor::InYellowRangePreProcessor( const cv::Mat& Src )
{
    cv::Mat SrcHSV;
    cvtColor( Src, SrcHSV, cv::COLOR_BGR2HSV );
    cv::inRange( SrcHSV, cv::Scalar( 25, 0, 150 ), cv::Scalar( 35, 255, 255 ), m_GrayImg );
    cvtColor( m_GrayImg, m_MatchVisualizerImg, cv::COLOR_GRAY2BGR );
}

std::vector<char>
EchoExtractor::MatchWithRecognizer( const cv::Mat&     Src,
                                    auto&              Recognizer,
                                    const std::string& MatchName,
                                    void ( EchoExtractor::*PreProcessor )( const cv::Mat& ) )
{
    ( this->*PreProcessor )( Src );

    auto& MatchRects = Recognizer.ExtractRect( m_GrayImg, m_MatchVisualizerImg );

    if ( !MatchName.empty( ) )
    {
        const int MinimumWidth  = 300;
        const int MinimumHeight = 300;
        const int FinalWidth    = std::max( m_MatchVisualizerImg.cols + 100, MinimumWidth );

        cv::Mat newImage = cv::Mat::zeros( std::max( m_MatchVisualizerImg.rows, MinimumHeight ), FinalWidth, m_MatchVisualizerImg.type( ) );
        m_MatchVisualizerImg.copyTo( newImage( cv::Rect( 0, 0, m_MatchVisualizerImg.cols, m_MatchVisualizerImg.rows ) ) );

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
        std::vector<cv::Rect> Overlaps( { CurrentMatch.rect } );
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

            cv::Mat matROI = m_GrayImg( cv::Rect { left, top, right - left, bottom - top } );

            cv::Mat matROIResized;
            RecognizerBase<void>::PixelPerfectResize( matROI, matROIResized );

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

std::vector<char>
EchoExtractor::MatchText( const cv::Mat& Src, const std::string& MatchName )
{
    return MatchWithRecognizer( Src, m_CRecognizer, MatchName );
}

std::vector<char>
EchoExtractor::MatchType( const cv::Mat& Src, const std::string& MatchName )
{
    return MatchWithRecognizer( Src, m_TRecognizer, MatchName );
}

std::vector<char>
EchoExtractor::MatchCost( const cv::Mat& Src, const std::string& MatchName )
{
    return MatchWithRecognizer( Src, m_TRecognizer, MatchName, &EchoExtractor::CostThresholdPreProcessor );
}

std::vector<char>
EchoExtractor::MatchEchoName( const cv::Mat& Src, const std::string& MatchName )
{
    return MatchWithRecognizer( Src, m_NRecognizer, MatchName, &EchoExtractor::InYellowRangePreProcessor );
}

FullStats
EchoExtractor::ExtractStat( const cv::Mat& Cimg, const cv::Mat& Timg, const cv::Mat& Nimg )
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
        std::filesystem::path TemplateName = m_NRecognizer.GetTemplates( )[ NResult.front( ) ].template_file;
        Result.EchoName                    = TemplateName.stem( ).string( );
    }

    return Result;
}

FullStats
EchoExtractor::ReadCard( const cv::Mat& Src )
{
    const cv::Rect NameRect { 880, 115, 200, 25 };
    const auto     NameImage = Src( NameRect );

    const cv::Rect CostRect { 1148, 170, 76, 18 };
    const auto     CostImage = Src( CostRect );

    const cv::Rect LevelRect { 1202, 195, 28, 22 };
    const auto     LevelImage = Src( LevelRect );

    const cv::Rect StatRect { 885, 305, 350, 220 };
    const auto     StatImage = Src( StatRect );

    const cv::Rect TypeRect { 0, 0, 26, 220 };
    const auto     TypeImage = StatImage( TypeRect );

    const cv::Rect NumberRect { 1150 - 885, 0, 80, 220 };
    const auto     NumberImage = StatImage( NumberRect );

    auto FS = ExtractStat( NumberImage, TypeImage, NameImage );
    if ( int Cost = ExtractCost( CostImage ); Cost != 0 )
    {
        FS.Cost = Cost;
    } else
    {
        return { };
    }

    int SetColorCoordinateX = 977, SetColorCoordinateY = 242;
    FS.Set = MatchColorToSet( { Src.data[ Src.channels( ) * ( Src.cols * SetColorCoordinateY + SetColorCoordinateX ) + 2 ],
                                Src.data[ Src.channels( ) * ( Src.cols * SetColorCoordinateY + SetColorCoordinateX ) + 1 ],
                                Src.data[ Src.channels( ) * ( Src.cols * SetColorCoordinateY + SetColorCoordinateX ) + 0 ] } );


    const auto LevelChars = MatchText( LevelImage, "Level" );
    std::from_chars( LevelChars.data( ), LevelChars.data( ) + LevelChars.size( ), FS.Level, 10 );
    if ( FS.Level > 25 )
    {
        std::cout << "Level is invalid: " << FS.Level << std::endl;
        return { };
    }

    return FS;
}

int
EchoExtractor::ExtractCost( const cv::Mat& Timg )
{
    const auto CostChars = MatchCost( Timg );
    if ( CostChars.empty( ) || CostChars[ 0 ] < '1' || CostChars[ 0 ] > '4' )
    {
        std::cout << "Cost is invalid: " << ( CostChars.empty( ) ? "null" : std::string( 1, CostChars[ 0 ] ) ) << " (" << CostChars.size( ) << ")" << std::endl;
        return 0;
    }

    return CostChars[ 0 ] - '0';
}