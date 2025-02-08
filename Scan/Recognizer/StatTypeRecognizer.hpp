//
// Created by EMCJava on 7/15/2024.
//

#pragma once

#include "RecognizerBase.hpp"

#include <Common/Stat/StatType.hpp>

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
            Template {                    (char) StatType::eAttack,       "attack.png", cv::Scalar( 255, 248, 240 ), 200},
            Template {(char) StatType::eAutoAttackDamagePercentage,  "auto_attack.png", cv::Scalar( 255, 255,   0 ), 180},
            Template {     (char) StatType::eHeavyAttackPercentage, "heavy_attack.png", cv::Scalar( 193, 182, 255 ), 200},
            Template {       (char) StatType::eUltDamagePercentage,   "ult_damage.png", cv::Scalar( 193, 182, 255 ), 180},
            Template {     (char) StatType::eSkillDamagePercentage,    "skill_dmg.png", cv::Scalar( 255,   0, 245 ), 210},
            Template {       (char) StatType::eHealBonusPercentage,   "heal_bonus.png", cv::Scalar( 160, 158,  95 ), 200},
            Template {      (char) StatType::eFireDamagePercentage,     "fire_dmg.png",  cv::Scalar( 80, 127, 255 ), 200},
            Template {       (char) StatType::eAirDamagePercentage,      "air_dmg.png", cv::Scalar( 169, 169, 169 ), 200},
            Template {       (char) StatType::eIceDamagePercentage,      "ice_dmg.png", cv::Scalar( 107, 183, 189 ), 200},
            Template {  (char) StatType::eElectricDamagePercentage,     "elec_dmg.png", cv::Scalar( 204,  50, 153 ), 200},
            Template {      (char) StatType::eDarkDamagePercentage,     "dark_dmg.png", cv::Scalar( 240, 255, 255 ), 200},
            Template {     (char) StatType::eLightDamagePercentage,    "light_dmg.png", cv::Scalar( 255, 144,  30 ), 200},
            Template {                   (char) StatType::eDefence,      "defence.png",  cv::Scalar( 47, 255, 173 ), 180},
            Template {                    (char) StatType::eHealth,       "health.png",   cv::Scalar( 0, 255, 255 ), 200},
            Template {           (char) StatType::eRegenPercentage,        "regen.png",   cv::Scalar( 0, 191, 255 ), 200},
            Template {                (char) StatType::eCritDamage,     "crit_dmg.png", cv::Scalar( 153,  50, 204 ), 200},
            Template {                  (char) StatType::eCritRate,    "crit_rate.png", cv::Scalar( 205, 133,  63 ), 220},
            Template {                                         '1',       "cost_1.png", cv::Scalar( 193, 182, 255 ), 250},
            Template {                                         '3',       "cost_3.png", cv::Scalar( 255, 240, 245 ), 250},
            Template {                                         '4',       "cost_4.png", cv::Scalar( 160, 158,  95 ), 250},
    };
};
