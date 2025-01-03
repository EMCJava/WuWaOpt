//
// Created by EMCJava on 1/3/2025.
//

#pragma once

#include "RecognizerBase.hpp"

#include <Common/Stat/EchoSet.hpp>

class SetRecognizer : public RecognizerBase<SetRecognizer>

{
public:
    SetRecognizer( )
    {
        Initialize( );
    }

    [[nodiscard]] const auto& GetTemplates( ) const
    {
        return Templates;
    }

private:
    std::array<Template, 14>
        Templates {
            Template {      static_cast<uint8_t>( EchoSet::eFreezingFrost ),       "template_set_img\\eFreezingFrost.png", cv::Scalar( 255, 248, 240 ), 240, false},
            Template {         static_cast<uint8_t>( EchoSet::eMoltenRift ),          "template_set_img\\eMoltenRift.png", cv::Scalar( 255, 255,   0 ), 240, false},
            Template {        static_cast<uint8_t>( EchoSet::eVoidThunder ),         "template_set_img\\eVoidThunder.png", cv::Scalar( 193, 182, 255 ), 240, false},
            Template {         static_cast<uint8_t>( EchoSet::eSierraGale ),          "template_set_img\\eSierraGale.png", cv::Scalar( 193, 182, 255 ), 240, false},
            Template {     static_cast<uint8_t>( EchoSet::eCelestialLight ),      "template_set_img\\eCelestialLight.png", cv::Scalar( 160, 158,  95 ), 240, false},
            Template {  static_cast<uint8_t>( EchoSet::eSunSinkingEclipse ),   "template_set_img\\eSunSinkingEclipse.png",  cv::Scalar( 80, 127, 255 ), 240, false},
            Template {   static_cast<uint8_t>( EchoSet::eRejuvenatingGlow ),    "template_set_img\\eRejuvenatingGlow.png", cv::Scalar( 169, 169, 169 ), 240, false},
            Template {      static_cast<uint8_t>( EchoSet::eMoonlitClouds ),       "template_set_img\\eMoonlitClouds.png", cv::Scalar( 107, 183, 189 ), 240, false},
            Template {     static_cast<uint8_t>( EchoSet::eLingeringTunes ),      "template_set_img\\eLingeringTunes.png", cv::Scalar( 204,  50, 153 ), 240, false},
            Template {      static_cast<uint8_t>( EchoSet::eFrostyResolve ),       "template_set_img\\eFrostyResolve.png", cv::Scalar( 240, 255, 255 ), 240, false},
            Template {    static_cast<uint8_t>( EchoSet::eEternalRadiance ),     "template_set_img\\eEternalRadiance.png", cv::Scalar( 255, 144,  30 ), 240, false},
            Template {       static_cast<uint8_t>( EchoSet::eMidnightVeil ),        "template_set_img\\eMidnightVeil.png",  cv::Scalar( 47, 255, 173 ), 240, false},
            Template {     static_cast<uint8_t>( EchoSet::eEmpyreanAnthem ),      "template_set_img\\eEmpyreanAnthem.png", cv::Scalar( 183, 255,  50 ), 240, false},
            Template {static_cast<uint8_t>( EchoSet::eTidebreakingCourage ), "template_set_img\\eTidebreakingCourage.png", cv::Scalar( 169,  50,  50 ), 240, false},
    };
};
