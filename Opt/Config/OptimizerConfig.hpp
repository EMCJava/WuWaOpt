//
// Created by EMCJava on 7/25/2024.
//

#pragma once

#include <Common/Stat/EffectiveStats.hpp>

#include <Loca/Loca.hpp>

struct OptimizerConfig {
    Language LastUsedLanguage = Language::Undefined;

    // + 1 when saved to file
    int InternalStateID = 0;

    bool AskedCheckForNewVersion = false;
    bool ShouldCheckForNewVersion = false;

    int OptimizerVersionCode = 0;

    void ReadConfig( );
    void SaveConfig( );
    void VersionUpgrade();
};
