// $Id: ThreadPool.h,v 1.1.1.1 2013/01/06 16:54:16 braunefe Exp $

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

#ifndef moses_ThreadPool_h
#define moses_ThreadPool_h

#include <iostream>
#include <queue>
#include <vector>

#ifdef WITH_THREADS
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#endif

#ifdef BOOST_HAS_PTHREADS
#include <pthread.h>
#endif


//#include "Util.h"


/**
  * Classes to implement a ThreadPool.
  **/

namespace Moses
{


/**
* A task to be executed by the ThreadPool
**/
class Task
{
public:
  virtual void Run() = 0;
  virtual bool DeleteAfterExecution() {return true;}
  virtual ~Task() {}
};

//#ifdef WITH_THREADS

class ThreadPool
{
public:
  /**
    * Construct a thread pool of a fixed size.
    **/
  ThreadPool(size_t numThreads);


  /**
   * Add a job to the threadpool.
   **/
  void Submit(Task* task);

  /**
    * Wait until all queued jobs have completed, and shut down
    * the ThreadPool.
    **/
  void Stop(bool processRemainingJobs = false);

  ~ThreadPool() {
    Stop();
  }



private:
  /**
    * The main loop executed by each thread.
    **/
  void Execute();

  std::queue<Task*> m_tasks;
  boost::thread_group m_threads;
  boost::mutex m_mutex;
  boost::condition_variable m_threadNeeded;
  boost::condition_variable m_threadAvailable;
  bool m_stopped;
  bool m_stopping;

};


class TestTask : public Task
{
public:
  TestTask(int id) : m_id(id) {}
  virtual void Run() {
#ifdef BOOST_HAS_PTHREADS
    pthread_t tid = pthread_self();
#else
    pthread_t tid = 0;
#endif
    std::cerr << "Executing " << m_id << " in thread id " << tid << std::endl;
  }

  virtual ~TestTask() {}

private:
  int m_id;
};

//#endif //WITH_THREADS

}
#endif
