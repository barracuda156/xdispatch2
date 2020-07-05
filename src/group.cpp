/*
* group.cpp
*
* Copyright (c) 2011-2018 MLBA-Team
* All rights reserved.
*
* @LICENSE_HEADER_START@
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
* @LICENSE_HEADER_END@
*/


#include "xdispatch_internal.h"
#include "xdispatch/igroup_impl.h"
#include "xdispatch/iqueue_impl.h"

__XDISPATCH_USE_NAMESPACE


group::group(
    const igroup_impl_ptr& impl
)
    : m_impl( impl )
{
    XDISPATCH_ASSERT( m_impl );
}


void group::async(
    const operation_ptr& op,
    const queue& q
)
{
    XDISPATCH_ASSERT( op );

    const auto q_impl = q.implementation();
    if( m_impl->backend() == q_impl->backend() )
    {
        queue_operation_with_d( *op, q_impl.get() );
        m_impl->async( op, q_impl );
    }
    else
    {
        throw std::logic_error( "Cannot mix two different backends" );
    }
}


void group::notify(
    const operation_ptr& op,
    const queue& q
)
{
    XDISPATCH_ASSERT( op );

    const auto q_impl = q.implementation();
    if( m_impl->backend() == q_impl->backend() )
    {
        queue_operation_with_d( *op, q_impl.get() );
        m_impl->notify( op, q_impl );
    }
    else
    {
        throw std::logic_error( "Cannot mix two different backends" );
    }
}

bool group::wait(
    std::chrono::milliseconds t
)
{
    return m_impl->wait( t );
}
