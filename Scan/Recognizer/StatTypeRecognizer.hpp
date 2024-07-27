//
// Created by EMCJava on 7/15/2024.
//

#pragma once

#include "RecognizerBase.hpp"

#include <Opt/SetStat.hpp>

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
            Template {                    eAttack,       "attack.png", cv::Scalar( 255, 248, 240 ), 200},
            Template {eAutoAttackDamagePercentage,  "auto_attack.png", cv::Scalar( 255, 255,   0 ), 180},
            Template {     eHeavyAttackPercentage, "heavy_attack.png", cv::Scalar( 193, 182, 255 ), 200},
            Template {       eUltDamagePercentage,   "ult_damage.png", cv::Scalar( 193, 182, 255 ), 180},
            Template {     eSkillDamagePercentage,    "skill_dmg.png", cv::Scalar( 255,   0, 245 ), 200},
            Template {       eHealBonusPercentage,   "heal_bonus.png", cv::Scalar( 160, 158,  95 ), 200},
            Template {      eFireDamagePercentage,     "fire_dmg.png",  cv::Scalar( 80, 127, 255 ), 200},
            Template {       eAirDamagePercentage,      "air_dmg.png", cv::Scalar( 169, 169, 169 ), 200},
            Template {       eIceDamagePercentage,      "ice_dmg.png", cv::Scalar( 107, 183, 189 ), 200},
            Template {  eElectricDamagePercentage,     "elec_dmg.png", cv::Scalar( 204,  50, 153 ), 200},
            Template {      eDarkDamagePercentage,     "dark_dmg.png", cv::Scalar( 240, 255, 255 ), 200},
            Template {     eLightDamagePercentage,    "light_dmg.png", cv::Scalar( 255, 144,  30 ), 200},
            Template {                   eDefence,      "defence.png",  cv::Scalar( 47, 255, 173 ), 180},
            Template {                    eHealth,       "health.png",   cv::Scalar( 0, 255, 255 ), 200},
            Template {           eRegenPercentage,        "regen.png",   cv::Scalar( 0, 191, 255 ), 200},
            Template {                eCritDamage,     "crit_dmg.png", cv::Scalar( 153,  50, 204 ), 200},
            Template {                  eCritRate,    "crit_rate.png", cv::Scalar( 205, 133,  63 ), 200},
            Template {                        '1',       "cost_1.png", cv::Scalar( 193, 182, 255 ), 250},
            Template {                        '3',       "cost_3.png", cv::Scalar( 255, 240, 245 ), 250},
            Template {                        '4',       "cost_4.png", cv::Scalar( 160, 158,  95 ), 250},
    };
};
