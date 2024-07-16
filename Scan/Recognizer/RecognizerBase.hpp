//
// Created by EMCJava on 7/15/2024.
//

#pragma once

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/ml/ml.hpp>

#include <filesystem>
#include <queue>

#include <spdlog/spdlog.h>

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
            spdlog::error( "Error reading template file: {} ", template_file );
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

template <typename Child>
class RecognizerBase
{
public:
    static constexpr int RESIZED_IMAGE_WIDTH  = 1200;
    static constexpr int RESIZED_IMAGE_HEIGHT = 300;

    void Initialize( )
    {
        KNearestModel = TrainKNearest( ( (Child*) this )->GetTemplates( ) );
    }

    static void
    PixelPerfectResize( const cv::Mat& src, cv::Mat& dst, int width = RESIZED_IMAGE_WIDTH, int height = RESIZED_IMAGE_HEIGHT )
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

    static auto
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

    auto& ExtractRect( const cv::Mat& Src, const cv::Mat& MatchVisualizer )
    {
        while ( !MatchRects.empty( ) )
            MatchRects.pop( );

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
