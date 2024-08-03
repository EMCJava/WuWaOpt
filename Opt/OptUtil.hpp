//
// Created by EMCJava on 6/16/2024.
//

#pragma once

using FloatTy = float;

#include "FullStats.hpp"

#include <source_location>
#include <sstream>
#include <chrono>
#include <queue>
#include <set>

#include <spdlog/spdlog.h>

class Stopwatch
{
public:
    Stopwatch( std::source_location loc = std::source_location::current( ) )
        : start_time( std::chrono::high_resolution_clock::now( ) )
        , m_SourceLocation( loc )
    { }

    ~Stopwatch( )
    {
        auto end_time = std::chrono::high_resolution_clock::now( );
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end_time - start_time ).count( );
        spdlog::info( "[{}] Elapsed time: {} seconds", m_SourceLocation.function_name( ), duration / 1000'000.f );
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    std::source_location m_SourceLocation;
};

template <std::size_t Count>
struct BoolArray {
    std::array<uint8_t, Count / 8 + 1> Value { 0 };
    int                                TrueCount = 0;

    constexpr void Reset( )
    {
        TrueCount = 0;
        memset( Value.data( ), 0, sizeof( Value ) );
    }

    constexpr bool GetAt( int Index ) const
    {
        return ( Value[ Index / 8 ] >> ( Index % 8 ) );
    }

    template <bool Data = true>
    constexpr bool SetAt( int Index )
    {
        auto&      Slot      = Value[ Index / 8 ];
        const auto BitAtSlot = 1 << ( Index % 8 );

        if constexpr ( Data )
        {
            if ( ~Slot & BitAtSlot )
            {
                Slot |= BitAtSlot;
                TrueCount += 1;
                return true;
            }
        } else
        {
            if ( Slot & BitAtSlot )
            {
                Slot &= ~BitAtSlot;
                TrueCount -= 1;
                return true;
            }
        }

        return false;
    }
};

struct CombinationRecord {
    uint64_t CombinationData = 0;
    FloatTy  Value           = 0;
    int      SlotCount       = 5;

    static constexpr auto IndexMaskBitCount = 12;
    static constexpr auto IndexMask         = ( (uint64_t) 1 << IndexMaskBitCount ) - 1;

    constexpr auto& SetAt( uint64_t Data, int Slot ) noexcept
    {
        assert( Data >= 0 && Data <= IndexMask );
        assert( Slot >= 0 && Slot < SlotCount );

        const auto Shift = Slot * IndexMaskBitCount;
        CombinationData &= ~( IndexMask << Shift );
        CombinationData |= Data << Shift;

        return *this;
    }

    [[nodiscard]] constexpr auto GetAt( int Slot ) const noexcept
    {
        assert( Slot >= 0 && Slot < SlotCount );

        const auto Shift = Slot * IndexMaskBitCount;
        return ( CombinationData & ( IndexMask << Shift ) ) >> Shift;
    }

    constexpr bool operator( )( CombinationRecord& obj1, CombinationRecord& obj2 ) const noexcept
    {
        return obj1.Value > obj2.Value;
    }

    constexpr std::array<int, 5> SlotToArray( ) const noexcept
    {
        std::array<int, 5> Result { -1, -1, -1, -1, -1 };
        for ( int i = 0; i < SlotCount; ++i )
        {
            if ( GetAt( i ) == IndexMask ) break;
            Result[ i ] = GetAt( i );
        }

        return Result;
    }

    [[nodiscard]] std::string ToString( ) const
    {
        std::stringstream ss;
        ss << Value << " : ";
        for ( int i = 0; i < SlotCount; ++i )
        {
            if ( GetAt( i ) == IndexMask ) break;

            ss << GetAt( i ) << " -> ";
        }
        ss << "nul";

        return ss.str( );
    }
};