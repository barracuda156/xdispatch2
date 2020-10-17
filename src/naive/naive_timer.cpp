/*
* Copyright (c) 2011-2013 MLBA-Team. All rights reserved.
*
* @MLBA_OPEN_LICENSE_HEADER_START@
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* @MLBA_OPEN_LICENSE_HEADER_END@
*/

#include "xdispatch/itimer_impl.h"
#include "xdispatch/iqueue_impl.h"

#include "naive_threadpool.h"
#include "naive_inverse_lockguard.h"

__XDISPATCH_BEGIN_NAMESPACE
namespace naive
{

class timer_impl : public itimer_impl, public std::enable_shared_from_this<timer_impl>
{
public:
    timer_impl(
        const iqueue_impl_ptr& queue,
        backend_type backend
    ) : itimer_impl()
        , m_backend( backend )
        , m_interval( 0 )
        , m_queue( queue )
        , m_handler()
        , m_running( false )
    {
    }

    ~timer_impl() final
    {
        stop();
    }

    void interval(
        std::chrono::milliseconds interval
    ) final
    {
        std::lock_guard< std::mutex > lock( m_CS );
        m_interval = interval;
    }

    void latency(
        timer_precision /* precision */
    ) final
    {
    }

    void handler(
        const operation_ptr& op
    ) final
    {
        std::lock_guard< std::mutex > lock( m_CS );
        m_handler = op;
    }

    void target_queue(
        const iqueue_impl_ptr& q
    ) final
    {
        std::lock_guard< std::mutex > lock( m_CS );
        m_queue = q;
    }

    void start(
        std::chrono::milliseconds delay
    ) final
    {
        std::lock_guard< std::mutex > lock( m_CS );
        m_running = true;

        const auto this_ptr = shared_from_this();

        // the timer will execute via a helper borrowed from the
        // global default threadpool. It is ensured that enough
        // threads are available for the pool even though the
        // timer is blocking while it is active
        auto timer_op = make_operation( [this_ptr, delay]
        {
            threadpool::instance()->notify_thread_blocked();
            std::this_thread::sleep_for( delay );

            std::unique_lock< std::mutex > lock( this_ptr->m_CS );
            while( this_ptr->m_running )
            {
                const auto handler = this_ptr->m_handler;
                const auto interval = this_ptr->m_interval;
                const auto queue = this_ptr->m_queue;

                inverse_lock_guard< std::mutex >unlock( this_ptr->m_CS );
                queue->async( handler );

                std::this_thread::sleep_for( interval );
            }
            threadpool::instance()->notify_thread_unblocked();
        } );

        // FIXME(zwicker): Add accessors to execute with the queue's priority
        threadpool::instance()->execute( std::move( timer_op ), queue_priority::DEFAULT );
    }

    void stop()
    {
        std::lock_guard< std::mutex > lock( m_CS );
        m_running = false;
    }

    backend_type backend() final
    {
        return m_backend;
    }

private:
    const backend_type m_backend;
    std::mutex m_CS;
    std::chrono::milliseconds m_interval;
    iqueue_impl_ptr m_queue;
    operation_ptr m_handler;
    bool m_running;
};

itimer_impl_ptr backend::create_timer(
    const iqueue_impl_ptr& queue,
    backend_type backend
)
{
    return std::make_shared< timer_impl >( queue, backend );
}

}
__XDISPATCH_END_NAMESPACE
