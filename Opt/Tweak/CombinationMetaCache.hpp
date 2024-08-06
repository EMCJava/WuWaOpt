//
// Created by EMCJava on 7/26/2024.
//

#pragma once

#include <Common/Stat/FullStats.hpp>

#include <Opt/UI/Backpack.hpp>
#include <Opt/UI/PlotCombinationMeta.hpp>
#include <Opt/Config/CharacterConfig.hpp>

class CombinationMetaCache
{
    const std::vector<EffectiveStats>& EffectiveEchoList;

    // This should only be used for comparing caches
    std::array<int, 5> m_CombinationEchoIndices { };

    std::vector<EffectiveStats> m_Echoes { };
    std::vector<FullStats>      m_FullEchoes { };
    std::vector<std::string>    m_EchoNames { };

    int            m_CachedStateID { };
    EffectiveStats m_CommonStats { };
    EffectiveStats m_CombinationStats { };

    CharacterConfig m_CharacterCfg;

    // After taking off echo at index
    std::array<std::vector<EffectiveStats>, 5> m_EchoesWithoutAt { };
    std::array<FloatTy, 5>                     m_EDWithoutAt { };
    std::array<FloatTy, 5>                     m_EdDropPercentageWithoutAt { };

    // Mapped stats
    EffectiveStats m_DisplayStats { };

    // Increased Payoff per stat
    EffectiveStats m_IncreasePayOff { };
    EffectiveStats m_IncreasePayOffWeight { };

    FloatTy m_FinalAttack    = 0;
    FloatTy m_NormalDamage   = 0;
    FloatTy m_CritDamage     = 0;
    FloatTy m_ExpectedDamage = 0;

    int m_ElementOffset = 0;
    int SlotCount       = 0;

    bool m_Valid = false;

    void CalculateDamages( );

public:
    explicit CombinationMetaCache( const std::vector<EffectiveStats>& EffectiveEchoList );

    void SetAsCombination( const Backpack&            BackPack,
                           const EffectiveStats&      CS,
                           int                        EO,
                           const PlotCombinationMeta& CombinationDetails,
                           const CharacterConfig&     Config );

    FloatTy GetEDReplaceEchoAt( int EchoIndex, EffectiveStats Echo ) const;

    void               Deactivate( );
    [[nodiscard]] bool IsValid( ) const noexcept { return m_Valid; }

    [[nodiscard]] auto GetSlotCount( ) const noexcept { return SlotCount; }

    [[nodiscard]] auto GetFinalAttack( ) const noexcept { return m_FinalAttack; }
    [[nodiscard]] auto GetNormalDamage( ) const noexcept { return m_NormalDamage; }
    [[nodiscard]] auto GetCritDamage( ) const noexcept { return m_CritDamage; }
    [[nodiscard]] auto GetExpectedDamage( ) const noexcept { return m_ExpectedDamage; }

    [[nodiscard]] auto& GetEffectiveEchoAtSlot( int SlotIndex ) const noexcept { return m_Echoes[ SlotIndex ]; };
    [[nodiscard]] auto& GetFullEchoAtSlot( int SlotIndex ) const noexcept { return m_FullEchoes[ SlotIndex ]; };
    [[nodiscard]] auto& GetEchoNameAtSlot( int SlotIndex ) const noexcept { return m_EchoNames[ SlotIndex ]; };
    [[nodiscard]] auto  GetEdDropPercentageWithoutAt( int Index ) const noexcept { return m_EdDropPercentageWithoutAt[ Index ]; };

    [[nodiscard]] auto& GetIncreasePayOff( ) const noexcept { return m_IncreasePayOff; }
    [[nodiscard]] auto& GetIncreasePayOffWeight( ) const noexcept { return m_IncreasePayOffWeight; }
    [[nodiscard]] auto& GetDisplayStats( ) const noexcept { return m_DisplayStats; }

    bool operator==( const CombinationMetaCache& Other ) const noexcept
    {
        return m_Valid == Other.m_Valid && m_CachedStateID == Other.m_CachedStateID && m_ElementOffset == Other.m_ElementOffset && std::ranges::equal( m_CombinationEchoIndices, Other.m_CombinationEchoIndices );
    }

    bool operator!=( const CombinationMetaCache& Other ) const noexcept
    {
        return !( *this == Other );
    }
};
