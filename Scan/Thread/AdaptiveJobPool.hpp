//
// Created by LYS on 12/20/2024.
//


#pragma once

#include <functional>
#include <vector>
#include <thread>
#include <semaphore>
#include <queue>
#include <mutex>

class AdaptiveJobPool
{

    static constexpr ptrdiff_t MAX_WORKERS = 128;

    void Worker( std::stop_token StopToken, std::function<void( void* )>&& Job );

    void CreateNewWorker( );

public:
    AdaptiveJobPool( int MaxWorkers );

    auto GetWorkerCount( ) const noexcept { return m_ActiveWorkers; }

    void SetJobMaker( auto&& JobMaker ) { m_JobMaker = JobMaker; }

    void NewJob( void* Context );

    void Clear( );

private:
    int32_t m_ActiveWorkers = 0;
    int32_t m_MaxWorkers;

    std::function<std::function<void( void* )>( )> m_JobMaker;

    std::counting_semaphore<> m_ContextSem;
    std::mutex                m_ContextLock;
    std::queue<void*>         m_ContextList;

    std::counting_semaphore<MAX_WORKERS>       m_FreeSem;
    std::mutex                                 m_WorkerLock;
    std::vector<std::unique_ptr<std::jthread>> m_Workers;
};
