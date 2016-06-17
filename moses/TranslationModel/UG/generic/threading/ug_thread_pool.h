// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil -*-
#pragma once
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>

namespace ug {
class ThreadPool 
{
  typedef boost::asio::io_service service_t;
  service_t m_service;
  boost::thread_group m_pool;
  boost::scoped_ptr<service_t::work>  m_busywork;
  std::vector<boost::thread*> m_workers;
  boost::mutex m_lock;

public:
  ThreadPool(size_t const num_workers=0);
  ~ThreadPool();

  template<class return_type>
  boost::unique_future<return_type>
  add(boost::shared_ptr<boost::packaged_task<return_type> > job) 
  { 
    boost::lock_guard<boost::mutex> lock(m_lock);
    if (!m_busywork) throw "Thred pool isn't running!";

    typedef boost::packaged_task<return_type> callable;
    boost::unique_future<return_type> ret = job->get_future();
    m_service.post(boost::bind(&callable::operator(), job)); 
    return ret;
  }
  
  template<class callable>
  bool add(callable& job) 
  { 
    boost::lock_guard<boost::mutex> lock(m_lock);
    if (!m_busywork) return false;
    m_service.post(job); 
    return true;
  }

  void stop();
  void start(size_t const num_workers);
}; // end of class declaration ThreadPool
} // end of namespace ug
