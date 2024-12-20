//
// Created by LYS on 12/20/2024.
//

#include "AdaptiveJobPool.hpp"

#include <spdlog/spdlog.h>

void
AdaptiveJobPool::Worker( std::stop_token StopToken, std::function<void( void* )>&& Job )
{
    m_FreeSem.acquire( );

    while ( true )
    {
        m_FreeSem.release( );
        m_ContextSem.acquire( );
        m_FreeSem.acquire( );

        if ( StopToken.stop_requested( ) ) break;

        void* Context;

        {
            std::lock_guard CL( m_ContextLock );
            Context = m_ContextList.front( );
            m_ContextList.pop( );
        }

        spdlog::trace( "Job {} assigned to worker", Context );
        Job( Context );
        spdlog::trace( "Worker completed the job" );
    }
}

void
AdaptiveJobPool::CreateNewWorker( )
{
    if ( !m_JobMaker ) return;

    if ( m_ActiveWorkers >= m_MaxWorkers )
        return;

    std::lock_guard WL( m_WorkerLock );

    m_Workers[ m_ActiveWorkers ] = std::make_unique<std::jthread>( [ this ]( std::stop_token s ) { Worker( s, std::move( m_JobMaker( ) ) ); } );
    m_ActiveWorkers++;

    m_FreeSem.release( );
}

AdaptiveJobPool::AdaptiveJobPool( int MaxWorkers )
    : m_MaxWorkers( MAX_WORKERS > MaxWorkers ? MaxWorkers : MAX_WORKERS )
    , m_ContextSem( 0 )
    , m_FreeSem( 0 )
{
    m_Workers.resize( m_MaxWorkers );
}

void
AdaptiveJobPool::NewJob( void* Context )
{
    if ( !m_JobMaker ) return;

    if ( !m_FreeSem.try_acquire( ) )
    {
        CreateNewWorker( );

        // Make sure list won't grow indefinitely
        m_FreeSem.acquire( );
    }

    {
        std::lock_guard CL( m_ContextLock );
        m_ContextList.push( Context );
    }

    m_FreeSem.release( );
    m_ContextSem.release( );
}

void
AdaptiveJobPool::Clear( )
{
    std::lock_guard WL( m_WorkerLock );

    // Wait for all job tobe finish
    while ( m_ContextSem.try_acquire( ) )
    {
        m_ContextSem.release( );
        std::this_thread::yield( );
    }

    // Wait for all thread to be finish
    for ( int i = 0; i < m_ActiveWorkers; i++ )
        m_FreeSem.acquire( );

    // Allow them to terminate
    m_FreeSem.release( m_ActiveWorkers );

    for ( int i = 0; i < m_ActiveWorkers; i++ )
        m_Workers[ i ]->request_stop( );

    // notify all
    m_ContextSem.release( m_ActiveWorkers );

    m_Workers.clear( );
    m_ActiveWorkers = 0;
}
