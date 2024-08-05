//
// Created by EMCJava on 8/1/2024.
//

#pragma once

#include <Loca/Loca.hpp>
#include <Common/Stat/EffectiveStats.hpp>

#include <vector>

struct StatConstraint : public StatValueConfig {
    bool    Enabled    = false;
    FloatTy InputValue = { };
};

class EchoConstraint : public LanguageObserver
{
private:
    bool DisplayStatRow( StatConstraint& StatConfig, bool DefaultEnable );

public:
    EchoConstraint( Loca& LanguageProvider );

    void DisplayConstraintMenu( );

    bool operator( )( const EffectiveStats& TestStats ) const noexcept;

protected:
    std::vector<StatConstraint> m_AllConstraints;
    std::vector<StatConstraint> m_ActivatedConstraints;
};
