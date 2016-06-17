#include "ug_thread_pool.h"
#include <boost/foreach.hpp>
#include <iostream>

namespace ug {

ThreadPool::
ThreadPool(size_t const num_workers)
  : m_service()
{
  if (num_workers) start(num_workers);
}

void
ThreadPool::
start(size_t const num_workers)
{
  boost::lock_guard<boost::mutex> lock(m_lock);
  if (m_pool.size()) throw "Already running.";
  m_service.reset();
  m_busywork.reset(new service_t::work(m_service));
  m_workers.resize(num_workers);
  for (size_t i = 0; i < num_workers; ++i)
    {
      m_workers[i] 
        = m_pool.create_thread(boost::bind(&service_t::run, &m_service));
    }
}

void
ThreadPool::
stop()
{
  boost::lock_guard<boost::mutex> lock(m_lock);
  m_busywork.reset();
  m_pool.join_all();
  BOOST_FOREACH(boost::thread* t, m_workers)
    {
      m_pool.remove_thread(t);
      t->join();
      delete t;
    }
  m_workers.clear();
  m_service.stop();
}

ThreadPool::
~ThreadPool()
{
  // stop();
  m_busywork.reset();
  m_pool.join_all();
  m_service.stop();
  // std::cerr << "all good " << std::endl;
}

}
