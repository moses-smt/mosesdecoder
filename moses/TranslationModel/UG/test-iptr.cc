// -*- c++ -*-
#include <iostream>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include "generic/threading/ug_ref_counter.h"

using namespace std;

class X : public Moses::reference_counter
{
public:
  X() { cout << "hello" << endl; }
  ~X() { cout << "bye-bye" << endl; }
};

int main()
{
  boost::intrusive_ptr<X> i(new X);
  // i.reset();
  cout << "bla" << endl;
}
