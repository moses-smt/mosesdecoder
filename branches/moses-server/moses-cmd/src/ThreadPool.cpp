// $Id: FactorCollection.h 2413 2009-07-29 16:37:29Z bhaddow $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/


#include "ThreadPool.h"

using namespace std;
using namespace Moses;

template<typename R>
Moses::ThreadPool<R>::ThreadPool( size_t numThreads ) : m_terminate(false)
{
    for (size_t i = 0; i < numThreads; ++i) {
        m_threads.create_thread(boost::bind(&ThreadPool::Execute,this));
    }
}

template<typename R>
void Moses::ThreadPool<R>::Execute()
{
    do {
        Task<R>* task = NULL;
        { // Find a job to perform
            boost::mutex::scoped_lock lock(m_mutex);
            if (m_tasks.empty()) {
                m_condition.wait(lock);
            }
            if (!m_terminate && !m_tasks.empty()) {
                task = m_tasks.front();
                m_tasks.pop();
            }
        }
        //Execute job
        if (task) {
            task->Run();
            boost::mutex::scoped_lock lock(task->m_mutex);
            task->m_completed = true;
            task->m_condition.notify_all();
        }
    } while (!m_terminate);

}

template<typename R>
void Moses::ThreadPool<R>::Submit( Task<R> * task )
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_tasks.push(task);
    m_condition.notify_all();
     
}

template<typename R>
void Moses::ThreadPool<R>::Shutdown( )
{
    {
        boost::mutex::scoped_lock lock(m_mutex);
        m_terminate = true;
        m_condition.notify_all();
    }
    m_threads.join_all();
}

template<typename R>
void Moses::Task<R>::Wait( )
{
    boost::mutex::scoped_lock lock( m_mutex );
    m_condition.wait( lock, boost::bind( &Task<R>::m_completed, this ) );

}

template class Moses::ThreadPool<int>;
template class Moses::Task<int>;
