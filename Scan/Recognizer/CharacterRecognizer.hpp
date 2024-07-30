//
// Created by EMCJava on 7/15/2024.
//

#pragma once

#include "RecognizerBase.hpp"

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
            Template {'5',          "5.png", cv::Scalar( 107, 183, 189 ), 190},
            Template {'6',          "6.png", cv::Scalar( 204,  50, 153 ), 205},
            Template {'7',          "7.png", cv::Scalar( 240, 255, 255 ), 210},
            Template {'8',          "8.png", cv::Scalar( 255, 144,  30 ), 205},
            Template {'9',          "9.png",  cv::Scalar( 47, 255, 173 ), 205},
    };
};
