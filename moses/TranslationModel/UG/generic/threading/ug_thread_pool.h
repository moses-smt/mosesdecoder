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
  std::vector<boost::shared_ptr<boost::thread> > m_workers;

public:
  ThreadPool(size_t const num_workers);
  ~ThreadPool();

  template<class callable>
  void add(callable& job) { m_service.post(job); }
  
}; // end of class declaration ThreadPool
} // end of namespace ug
