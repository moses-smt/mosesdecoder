#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>
#include "moses/TranslationModel/UG/generic/threading/ug_thread_pool.h"
using namespace std;

class hello
{
  size_t n;
public:
  hello(size_t const x) : n(x) { }
  void operator()() { cout << "hello #" << n << endl; }
};


int main()
{
  ug::ThreadPool T(10);
  vector<boost::shared_ptr<hello> > jobs;
  for (size_t i = 0; i < 20; ++i)
    {
      boost::shared_ptr<hello> j(new hello(i));
      jobs.push_back(j);
      T.add(*j);
    }
}
