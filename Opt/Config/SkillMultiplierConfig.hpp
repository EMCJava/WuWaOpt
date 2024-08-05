//
// Created by EMCJava on 8/5/2024.
//

#pragma once

#include <Common/Types.hpp>

struct SkillMultiplierConfig {

    FloatTy auto_attack_multiplier  = 0;
    FloatTy heavy_attack_multiplier = 0;
    FloatTy skill_multiplier        = 0;
    FloatTy ult_multiplier          = 0;

    bool operator==( const SkillMultiplierConfig& Other ) const noexcept
    {
        return auto_attack_multiplier == Other.auto_attack_multiplier && heavy_attack_multiplier == Other.heavy_attack_multiplier && skill_multiplier == Other.skill_multiplier && ult_multiplier == Other.ult_multiplier;
    }
};
