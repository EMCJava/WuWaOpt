//
// Created by EMCJava on 7/25/2024.
//

#pragma once

#include <Opt/OptUtil.hpp>

struct ValueRollRate {
    using RateTy = double;

    FloatTy Value = { };
    RateTy  Rate  = { };
};

struct SubStatRollConfig {
    FloatTy EffectiveStats::*    ValuePtr      = { };
    std::array<ValueRollRate, 8> Values        = { };
    std::array<std::string, 8>   ValueStrings  = { };
    int                          PossibleRolls = 0;
    bool                         IsEffective   = false;

    void SetValueStrings( bool IsPercentage = true );

    static const char* GetValueString( void* user_data, int idx );
};
