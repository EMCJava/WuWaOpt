//
// Created by EMCJava on 6/16/2024.
//

#pragma once

#include <Common/Types.hpp>

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

template <typename Ty, size_t SlotCount>
struct CircularBuffer {
public:
    struct CircularBufferIterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = Ty;
        using pointer           = Ty*;
        using reference         = Ty&;

        CircularBufferIterator( pointer base_ptr = nullptr, size_t index = 0 )
            : m_base_ptr( base_ptr )
            , m_index( index )
        { }

        reference operator*( ) const { return m_base_ptr[ m_index ]; }
        pointer   operator->( ) { return m_base_ptr + m_index; }

        CircularBufferIterator& operator++( )
        {
            m_index++;
            if ( m_index >= SlotCount ) [[unlikely]]
                m_index -= SlotCount;
            return *this;
        }
        CircularBufferIterator operator++( int )
        {
            CircularBufferIterator tmp = *this;
            ++( *this );
            return tmp;
        }

        friend bool operator==( const CircularBufferIterator& a, const CircularBufferIterator& b ) { return a.m_base_ptr == b.m_base_ptr && a.m_index == b.m_index; }
        friend bool operator!=( const CircularBufferIterator& a, const CircularBufferIterator& b ) { return a.m_base_ptr != b.m_base_ptr || a.m_index != b.m_index; }

    private:
        const pointer m_base_ptr;
        size_t        m_index;
    };

    auto begin( ) { return CircularBufferIterator( m_Data ); }
    auto end( ) { return CircularBufferIterator( m_Data, SlotCount - 1 ); }

private:
    Ty m_Data[ SlotCount ];
};