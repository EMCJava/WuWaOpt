//
// Created by EMCJava on 7/6/2024.
//

#pragma once

#include <array>
#include <tuple>
#include <limits>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

enum EchoSet : uint8_t {
    eFreezingFrost,
    eMoltenRift,
    eVoidThunder,
    eSierraGale,
    eCelestialLight,
    eSunSinkingEclipses,
    eRejuvenatingGlow,
    eMoonlitClouds,
    eLingeringTunes,
    eEchoSetCount,
    eEchoSetNone = eEchoSetCount
};

NLOHMANN_JSON_SERIALIZE_ENUM( EchoSet, {
                                           {     eFreezingFrost,      "eFreezingFrost"},
                                           {        eMoltenRift,         "eMoltenRift"},
                                           {       eVoidThunder,        "eVoidThunder"},
                                           {        eSierraGale,         "eSierraGale"},
                                           {    eCelestialLight,     "eCelestialLight"},
                                           {eSunSinkingEclipses, "eSunSinkingEclipses"},
                                           {  eRejuvenatingGlow,   "eRejuvenatingGlow"},
                                           {     eMoonlitClouds,      "eMoonlitClouds"},
                                           {    eLingeringTunes,     "eLingeringTunes"},
                                           {       eEchoSetNone,        "eEchoSetNone"}
} )

constexpr std::array<std::tuple<int, int, int>, eEchoSetCount> EchoSetColorIndicators = {
    std::make_tuple( 66, 178, 255 ),
    std::make_tuple( 245, 118, 79 ),
    std::make_tuple( 182, 108, 255 ),
    std::make_tuple( 86, 255, 183 ),
    std::make_tuple( 247, 228, 107 ),
    std::make_tuple( 204, 141, 181 ),
    std::make_tuple( 135, 189, 41 ),
    std::make_tuple( 255, 255, 255 ),
    std::make_tuple( 202, 44, 37 ),
};


constexpr auto
ColorIndicatorsMinDifferent( ) noexcept
{
    int SmallestDistance = std::numeric_limits<int>::max( );
    for ( const auto& A : EchoSetColorIndicators )
    {
        for ( const auto& B : EchoSetColorIndicators )
        {
            const auto XDistance = std::get<0>( A ) - std::get<0>( B );
            const auto YDistance = std::get<1>( A ) - std::get<1>( B );
            const auto ZDistance = std::get<2>( A ) - std::get<2>( B );

            const auto Distance = XDistance * XDistance + YDistance * YDistance + ZDistance * ZDistance;
            if ( Distance != 0 && Distance < SmallestDistance )
            {
                SmallestDistance = Distance;
            }
        }
    }

    return SmallestDistance;
}

constexpr EchoSet
MatchColorToSet( std::tuple<int, int, int> Color ) noexcept
{
    int SmallestDistance   = std::numeric_limits<int>::max( );
    int SmallestDistanceAt = -1;
    for ( int i = 0; i < eEchoSetCount; ++i )
    {
        const auto XDistance = std::get<0>( EchoSetColorIndicators[ i ] ) - std::get<0>( Color );
        const auto YDistance = std::get<1>( EchoSetColorIndicators[ i ] ) - std::get<1>( Color );
        const auto ZDistance = std::get<2>( EchoSetColorIndicators[ i ] ) - std::get<2>( Color );

        const auto Distance = XDistance * XDistance + YDistance * YDistance + ZDistance * ZDistance;
        if ( Distance < SmallestDistance )
        {
            SmallestDistance   = Distance;
            SmallestDistanceAt = i;
        }
    }

    if ( SmallestDistance >= ColorIndicatorsMinDifferent( ) / 4 )
    {
        return eEchoSetNone;
    }

    return (EchoSet) SmallestDistanceAt;
}
