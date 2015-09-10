#include "ug_thread_pool.h"
namespace ug {

ThreadPool::
ThreadPool(size_t const num_workers)
  : m_service(), m_busywork(new boost::asio::io_service::work(m_service))
{
  m_workers.reserve(num_workers);
  for (size_t i = 0; i < num_workers; ++i)
    {
      // boost::shared_ptr<boost::thread> t;
      // t.reset(new boost::thread(boost::bind(&service_t::run, &m_service)));
      boost::thread* t;
      t = new boost::thread(boost::bind(&service_t::run, &m_service));
      m_pool.add_thread(t);
      // m_workers.push_back(t);
    }
}

ThreadPool::
~ThreadPool()
{
  m_busywork.reset();
  m_pool.join_all();
  m_service.stop();
}




}
