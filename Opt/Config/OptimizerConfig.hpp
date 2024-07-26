//
// Created by EMCJava on 7/25/2024.
//

#pragma once

#include <Opt/FullStats.hpp>

struct OptimizerConfig {
    EffectiveStats WeaponStats { };
    EffectiveStats CharacterStats { };

    MultiplierConfig OptimizeMultiplierConfig { };
    int              SelectedElement { };

    int     CharacterLevel { };
    int     EnemyLevel { };
    FloatTy ElementResistance { };
    FloatTy ElementDamageReduce { };

    Language LastUsedLanguage = Language::Undefined;

    // + 1 when saved to file
    int InternalStateID = 0;

    [[nodiscard]] FloatTy GetResistances( ) const noexcept;
    [[nodiscard]] FloatTy GetBaseAttack( ) const noexcept;

    void ReadConfig( );
    void SaveConfig( );
};
