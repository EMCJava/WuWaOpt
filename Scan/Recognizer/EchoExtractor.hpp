//
// Created by EMCJava on 7/15/2024.
//

#pragma once

#include "CharacterRecognizer.hpp"
#include "EchoNameRecognizer.hpp"
#include "StatTypeRecognizer.hpp"

#include <Common/Stat/FullStats.hpp>

class EchoExtractor
{
private:
    void ThresholdPreProcessor( const cv::Mat& Src );
    void TypeThresholdPreProcessor( const cv::Mat& Src );

    void InYellowRangePreProcessor( const cv::Mat& Src );

    void CullAndWriteProcessedGrayImage(const std::string& File);

    std::vector<char> MatchWithRecognizer( const cv::Mat&     Src,
                                           auto&              Recognizer,
                                           const std::string& MatchName                            = "",
                                           void ( EchoExtractor::*PreProcessor )( const cv::Mat& ) = &EchoExtractor::ThresholdPreProcessor );

    FullStats ExtractStat( const cv::Mat& Cimg, const cv::Mat& Timg, const cv::Mat& Nimg );
    int       ExtractCost( const cv::Mat& Timg );

public:
    std::vector<char> MatchText( const cv::Mat& Src, const std::string& MatchName = "" );
    std::vector<char> MatchType( const cv::Mat& Src, const std::string& MatchName = "" );
    std::vector<char> MatchCost( const cv::Mat& Src, const std::string& MatchName = "" );
    std::vector<char> MatchEchoName( const cv::Mat& Src, const std::string& MatchName = "" );

    FullStats ReadCard( const cv::Mat& Src );

private:
    StatTypeRecognizer  m_TRecognizer;
    CharacterRecognizer m_CRecognizer;
    EchoNameRecognizer  m_NRecognizer;

    /*
     *
     * Cache per match
     *
     * */
    cv::Mat m_GrayImg, m_MatchVisualizerImg;
};
