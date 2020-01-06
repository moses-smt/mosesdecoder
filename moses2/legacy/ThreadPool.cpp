// $Id: ThreadPool.cpp 3045 2010-04-05 13:07:29Z hieuhoang1972 $

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
#include <stdio.h>
#ifdef __linux
#include <pthread.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <thread>

#include "ThreadPool.h"

using namespace std;

namespace Moses2
{

#define handle_error_en(en, msg) \
  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

ThreadPool::ThreadPool(size_t numThreads, int cpuAffinityOffset,
                       int cpuAffinityIncr) :
  m_stopped(false), m_stopping(false), m_queueLimit(numThreads*2)
{
#if defined(_WIN32) || defined(_WIN64)
  size_t numCPU = std::thread::hardware_concurrency();
#else
  size_t numCPU = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  //cerr << "numCPU=" << numCPU << endl;

  int cpuInd = cpuAffinityOffset % numCPU;

  for (size_t i = 0; i < numThreads; ++i) {
    boost::thread *thread = m_threads.create_thread(
                              boost::bind(&ThreadPool::Execute, this));

#ifdef __linux
    if (cpuAffinityOffset >= 0) {
      int s;

      boost::thread::native_handle_type handle = thread->native_handle();

      //cerr << "numCPU=" << numCPU << endl;
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);

      CPU_SET(cpuInd, &cpuset);
      cpuInd += cpuAffinityIncr;
      cpuInd = cpuInd % numCPU;

      s = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
      if (s != 0) {
        handle_error_en(s, "pthread_setaffinity_np");
        //cerr << "affinity error with thread " << i << endl;
      }

      // get affinity
      CPU_ZERO(&cpuset);
      s = pthread_getaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
      cerr << "Set returned by pthread_getaffinity_np() contained:\n";
      for (int j = 0; j < CPU_SETSIZE; j++) {
        if (CPU_ISSET(j, &cpuset)) {
          cerr << "    CPU " << j << "\n";
        }
      }
    }
#endif
  }
}

void ThreadPool::Execute()
{
  do {
    boost::shared_ptr<Task> task;
    {
      // Find a job to perform
      boost::mutex::scoped_lock lock(m_mutex);
      if (m_tasks.empty() && !m_stopped) {
        m_threadNeeded.wait(lock);
      }
      if (!m_stopped && !m_tasks.empty()) {
        task = m_tasks.front();
        m_tasks.pop();
      }
    }
    //Execute job
    if (task) {
      // must read from task before run. otherwise task may be deleted by main thread
      // race condition
      task->DeleteAfterExecution();
      task->Run();
    }
    m_threadAvailable.notify_all();
  } while (!m_stopped);
}

void ThreadPool::Submit(boost::shared_ptr<Task> task)
{
  boost::mutex::scoped_lock lock(m_mutex);
  if (m_stopping) {
    throw runtime_error("ThreadPool stopping - unable to accept new jobs");
  }
  while (m_queueLimit > 0 && m_tasks.size() >= m_queueLimit) {
    m_threadAvailable.wait(lock);
  }
  m_tasks.push(task);
  m_threadNeeded.notify_all();
}

void ThreadPool::Stop(bool processRemainingJobs)
{
  {
    //prevent more jobs from being added to the queue
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_stopped) return;
    m_stopping = true;
  }
  if (processRemainingJobs) {
    boost::mutex::scoped_lock lock(m_mutex);
    //wait for queue to drain.
    while (!m_tasks.empty() && !m_stopped) {
      m_threadAvailable.wait(lock);
    }
  }
  //tell all threads to stop
  {
    boost::mutex::scoped_lock lock(m_mutex);
    m_stopped = true;
  }
  m_threadNeeded.notify_all();

  m_threads.join_all();
}

}

