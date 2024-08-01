//
// Created by EMCJava on 8/1/2024.
//

#include "EchoConstraint.hpp"

#include <imgui.h>
#include <imgui-SFML.h>

bool
EchoConstraint::DisplayStatRow( StatConstraint& StatConfig, bool DefaultEnable )
{
    ImGui::PushID( &StatConfig );

    bool Enable = DefaultEnable;
    ImGui::Checkbox( "", &Enable );
    StatConfig.Enabled = Enable;

    ImGui::SameLine( );

    if ( !DefaultEnable ) ImGui::BeginDisabled( );
    if ( ImGui::DragFloat(
             LanguageProvider[ EffectiveStats::GetStatName( StatConfig.ValuePtr ) ],
             &StatConfig.InputValue, 0.01, 0, 0, "%.2f" ) )
    {
        StatConfig.Value = StatConfig.InputValue / 100;
        if ( StatConfig.ValuePtr == &EffectiveStats::regen )
            StatConfig.Value -= EffectiveStats::CharacterDefaultRegen;
        else if ( StatConfig.ValuePtr == &EffectiveStats::crit_rate )
            StatConfig.Value -= EffectiveStats::CharacterDefaultCritRate;
        else if ( StatConfig.ValuePtr == &EffectiveStats::crit_damage )
            StatConfig.Value -= EffectiveStats::CharacterDefaultCritDamage;
    }
    if ( !DefaultEnable ) ImGui::EndDisabled( );

    ImGui::PopID( );

    return StatConfig.Enabled;
}

void
EchoConstraint::DisplayConstraintMenu( )
{
    bool HasDisabled = false;
    for ( auto& Constraint : m_ActivatedConstraints )
    {
        if ( !DisplayStatRow( Constraint, true ) )
        {
            auto ValueIt =
                std::ranges::find_if( m_AllConstraints, [ & ]( const StatConstraint& C ) {
                    return Constraint.ValuePtr == C.ValuePtr;
                } );
            *ValueIt    = Constraint;
            HasDisabled = true;
        }
    }

    if ( HasDisabled )
    {
        std::erase_if( m_ActivatedConstraints, [ & ]( const StatConstraint& C ) {
            return !C.Enabled;
        } );
    }

    if ( !m_ActivatedConstraints.empty( ) && m_ActivatedConstraints.size( ) != m_AllConstraints.size( ) )
    {
        ImGui::NewLine( );
        ImGui::Separator( );
        ImGui::NewLine( );
    }

    for ( auto& Constraint : m_AllConstraints )
    {
        if ( Constraint.Enabled ) continue;
        if ( DisplayStatRow( Constraint, false ) )
        {
            m_ActivatedConstraints.push_back( Constraint );
        }
    }
}

EchoConstraint::EchoConstraint( Loca& LanguageProvider )
    : LanguageObserver( LanguageProvider )
{
    m_AllConstraints = {
        StatConstraint { &EffectiveStats::buff_multiplier },
        StatConstraint { &EffectiveStats::auto_attack_buff },
        StatConstraint { &EffectiveStats::heavy_attack_buff },
        StatConstraint { &EffectiveStats::skill_buff },
        StatConstraint { &EffectiveStats::ult_buff },
        StatConstraint { &EffectiveStats::crit_rate },
        StatConstraint { &EffectiveStats::crit_damage },
        StatConstraint { &EffectiveStats::regen },
    };
}

bool
EchoConstraint::operator( )( const EffectiveStats& TestStats ) const noexcept
{
    for ( const auto& Constraint : m_ActivatedConstraints )
    {
        if ( TestStats.*( Constraint.ValuePtr ) < Constraint.Value ) return false;
    }

    return true;
}
