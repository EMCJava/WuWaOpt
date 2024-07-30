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
EchoExtractor::TypeThresholdPreProcessor( const cv::Mat& Src )
{
    cvtColor( Src, m_GrayImg, cv::COLOR_BGR2GRAY );
    cv::threshold( m_GrayImg, m_GrayImg, 106, 255, cv::THRESH_BINARY );
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

//    if ( !MatchName.empty( ) )
//    {
//        const int MinimumWidth  = 300;
//        const int MinimumHeight = 300;
//        const int FinalWidth    = std::max( m_GrayImg.cols + 100, MinimumWidth );
//
//        cv::Mat newImage = cv::Mat::zeros( std::max( m_GrayImg.rows, MinimumHeight ), FinalWidth, m_GrayImg.type( ) );
//        m_GrayImg.copyTo( newImage( cv::Rect( 0, 0, m_GrayImg.cols, m_GrayImg.rows ) ) );
//
//        if ( MatchName == "Stat Type" )
//        {
//            cv::namedWindow( MatchName, cv::WINDOW_NORMAL );
//            cv::resizeWindow( MatchName, newImage.cols, newImage.rows );
//            cv::imshow( MatchName, newImage );
//            cv::waitKey( 0 );
//        }
//    }

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
             && MatchRects.top( ).rect.y > CurrentMatch.rect.y + 15 )
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
    return MatchWithRecognizer( Src, m_TRecognizer, MatchName, &EchoExtractor::TypeThresholdPreProcessor );
}

std::vector<char>
EchoExtractor::MatchCost( const cv::Mat& Src, const std::string& MatchName )
{
    return MatchWithRecognizer( Src, m_TRecognizer, MatchName, &EchoExtractor::TypeThresholdPreProcessor );
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

    auto Types = TResult | std::views::split( '\n' ) | std::views::transform( []( auto&& c ) -> char {
                     if ( c.empty( ) )
                     {
                         spdlog::warn( "Warning: Empty type detected in EchoExtractor::ExtractStat" );
                         return '\0';
                     }
                     return c.front( );
                 } )
        | std::ranges::to<std::vector>( );
    auto Numbers = CResult | std::views::split( '\n' )
        | std::views::transform( []( auto&& Number ) -> FloatTy {   // parse
                       if ( Number.empty( ) )
                       {
                           spdlog::warn( "Warning: Empty number detected in EchoExtractor::ExtractStat" );
                           return 0;
                       }

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
                   } )
        | std::ranges::to<std::vector>( );

    if ( Types.size( ) != Numbers.size( ) )
    {
        throw std::runtime_error( "SubStat types and numbers do not match" );
    }

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
    spdlog::info( "Analyzing Src({}x{}) image...", Src.cols, Src.rows );

    const cv::Rect NameRect { 1310, 125, 280, 35 };
    const auto     NameImage = Src( NameRect );

    const cv::Rect CostRect { 1700, 210, 125, 25 };
    const auto     CostImage = Src( CostRect );

    const cv::Rect LevelRect { 1770, 250, 60, 30 };
    const auto     LevelImage = Src( LevelRect );

    const cv::Rect TypeRect { 1310, 420, 45, 320 };
    const auto     TypeImage = Src( TypeRect );

    const cv::Rect NumberRect { 1720, 420, 110, 320 };
    const auto     NumberImage = Src( NumberRect );

    auto FS = ExtractStat( NumberImage, TypeImage, NameImage );
    if ( int Cost = ExtractCost( CostImage ); Cost != 0 )
    {
        FS.Cost = Cost;
    } else
    {
        throw std::runtime_error( "Failed to extract cost" );
    }

    int SetColorCoordinateX = 1441, SetColorCoordinateY = 325;
    FS.Set = MatchColorToSet( { Src.data[ Src.channels( ) * ( Src.cols * SetColorCoordinateY + SetColorCoordinateX ) + 2 ],
                                Src.data[ Src.channels( ) * ( Src.cols * SetColorCoordinateY + SetColorCoordinateX ) + 1 ],
                                Src.data[ Src.channels( ) * ( Src.cols * SetColorCoordinateY + SetColorCoordinateX ) + 0 ] } );

    const auto LevelChars = MatchText( LevelImage, "Level" );
    std::from_chars( LevelChars.data( ), LevelChars.data( ) + LevelChars.size( ), FS.Level, 10 );
    if ( FS.Level > 25 )
    {
        throw std::runtime_error( "Level is invalid" );
    }

    return FS;
}

int
EchoExtractor::ExtractCost( const cv::Mat& Timg )
{
    const auto CostChars = MatchCost( Timg, "Cost" );
    if ( CostChars.empty( ) || CostChars[ 0 ] < '1' || CostChars[ 0 ] > '4' )
    {
        spdlog::error( "Cost is invalid: {} ({})", CostChars.empty( ) ? "null" : std::string( 1, CostChars[ 0 ] ), CostChars.size( ) );
        return 0;
    }

    return CostChars[ 0 ] - '0';
}