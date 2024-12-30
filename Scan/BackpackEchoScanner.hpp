//
// Created by EMCJava on 12/30/2024.
//

#pragma once

#include "Win/GameHandle.hpp"

#include <Common/Stat/FullStats.hpp>

#include <functional>

class BackpackEchoScanner
{
private:

    bool
    CheckResolution( );

    void Initialize( );

    std::unique_ptr<GameHandle> m_GameHandler;

    MouseControl::MousePoint m_CurrentMousePos { 2 };

    void MoveMouseToCard( FloatTy X, FloatTy Y );

    void SortBackpackEchoByLevel( );

public:
    BackpackEchoScanner( Loca& Localization );

    // CallBack return false to terminate
    void Scan( int EchoCount, const std::function<bool( const FullStats& )>& CallBack );

    void SetScanDelay( int ScanDelayMS );

private:
    MouseControl m_MouseController;

protected:
    bool          m_Initialized     = false;
    volatile bool m_ShouldTerminate = false;

    int m_ScanDelayMS = 300;

    Loca& m_LanguageProvider;
};
