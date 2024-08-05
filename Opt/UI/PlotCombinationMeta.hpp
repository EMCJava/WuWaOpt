//
// Created by EMCJava on 7/26/2024.
//

#pragma once

#include <Common/Types.hpp>

#include <implot.h>

#include <array>

struct PlotCombinationMeta {
    FloatTy            Value;
    int                CombinationID;
    int                CombinationRank;
    std::array<int, 5> Indices;

    bool operator>( PlotCombinationMeta Other ) const noexcept
    {
        return Value > Other.Value;
    }

    static ImPlotPoint
    PlotCombinationMetaImPlotGetter( int idx, void* user_data )
    {
        return { (double) idx, ( ( (PlotCombinationMeta*) user_data ) + idx )->Value };
    }

    static ImPlotPoint
    PlotCombinationMetaSegmentImPlotGetter( int idx, void* user_data )
    {
        return { std::round( (double) idx / 2 ), ( ( (PlotCombinationMeta*) user_data ) + idx )->Value };
    }
};