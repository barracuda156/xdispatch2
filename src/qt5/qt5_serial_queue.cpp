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

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QEvent>

#include "qt5_backend_internal.h"
#include "qt5_threadpool.h"
#include "../naive/naive_threadpool.h"

__XDISPATCH_BEGIN_NAMESPACE
namespace qt5
{

class ExecuteOperationEvent : public QEvent
{
public:
    static QEvent::Type Type()
    {
        static int sType = QEvent::registerEventType();
        return static_cast<QEvent::Type>( sType );
    }

    ExecuteOperationEvent(
        const operation_ptr& work
    )
        : QEvent( Type() )
        , m_work( work )
    {
    }

    void execute()
    {
        execute_operation_on_this_thread( *m_work );
    }

private:
    const operation_ptr m_work;
};

class ThreadProxy : public naive::ithreadpool, public QObject
{
public:
    explicit ThreadProxy(
        QThread* thread
    )
        : m_thread( thread )
    {
        XDISPATCH_ASSERT( m_thread );
        moveToThread( m_thread );
    }

    ~ThreadProxy()
    {
    }

    void execute(
        const operation_ptr& work,
        queue_priority /* priority */
    ) final
    {
        QCoreApplication::postEvent( this, new ExecuteOperationEvent( work ) );
    }

protected:
    void customEvent(
        QEvent* event
    ) final
    {
        if( ExecuteOperationEvent::Type() == event->type() )
        {
            static_cast<ExecuteOperationEvent*>( event )->execute();
        }

        QObject::customEvent( event );
    }

private:
    QThread* m_thread;
};

queue create_serial_queue(
    const std::string& label,
    QThread* thread,
    queue_priority priority
)
{
    XDISPATCH_ASSERT( thread );
    thread->setObjectName( QString::fromStdString( label ) );
    auto proxy = std::make_shared< ThreadProxy >( thread );
    return naive::create_serial_queue( label, std::move( proxy ), priority, backend_type::qt5 );
}

iqueue_impl_ptr backend::create_main_queue(
    const std::string& label
)
{
    auto instance = QCoreApplication::instance();
    if( nullptr == instance )
    {
        throw std::logic_error( "Construct a QCoreApplication before using the main queue" );
    }
    return qt5::create_serial_queue( label, instance->thread() ).implementation();
}

void backend::exec()
{
    auto instance = QCoreApplication::instance();
    if( nullptr == instance )
    {
        throw std::logic_error( "Construct a QCoreApplication before invoking exec()" );
    }
    instance->exec();
}

}
__XDISPATCH_END_NAMESPACE
