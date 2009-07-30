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

#pragma once

#include <iostream>
#include <queue>
#include <vector>

#include <boost/bind.hpp>
#include <boost/thread.hpp>


/**
  * Classes to implement a ThreadPool.
  **/

namespace Moses {

template<typename R> class ThreadPool;

/**
* A task to be executed by the ThreadPool
**/
template<typename R>
class Task {
    public:
        Task() : m_completed(false) {}
        R Result() {
            Wait();
            return m_result;
        }
        void Run() {m_result = doRun();}
        
        virtual ~Task() {}
        void Wait();
        friend class ThreadPool<R>;
    
   private:
        volatile bool m_completed;
        boost::mutex m_mutex;
        boost::condition m_condition;

   protected:
      virtual R doRun() = 0;
      R m_result;
}; 

template<typename R>
class ThreadPool {
    public:
        /**
          * Construct a thread pool of a fixed size.
          **/
        ThreadPool(size_t numThreads);


         /**
          * Add a job to the threadpool.
          **/
        void Submit(Task<R>* task);
        
        /**
          * Shutdown the ThreadPool
          **/
        void Shutdown();  
          
        ~ThreadPool() {}
        
        
        
    private:
        /**
          * The main loop executed by each thread.
          **/
        void Execute();
        
        std::queue<Task<R>*> m_tasks;
        boost::thread_group m_threads;
        boost::mutex m_mutex;
        boost::condition m_condition;
        bool m_terminate;
        
};

#include <pthread.h>

class TestTask : public Task<int> {
    public:
        TestTask(int id) : m_id(id) {}
        virtual int doRun() {
            int tid = (int)pthread_self();
            std::cerr << "Executing " << m_id << "in thread id " << tid << std::endl;
            return  m_id;
        }
        
        virtual ~TestTask() {}
        
    private:
        int m_id;
};



}
