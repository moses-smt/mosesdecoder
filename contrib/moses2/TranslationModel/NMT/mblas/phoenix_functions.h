#pragma once

#include <cmath>
#include <boost/phoenix/phoenix.hpp>

namespace mblas
{
  template <class T>
  auto Exp(const T& x) -> decltype(boost::phoenix::bind(exp, x))
  {
    return boost::phoenix::bind(exp, x);  
  }
  
  template <typename T>
  auto Tanh(const T& x) -> decltype(boost::phoenix::bind(tanh, x)) {
    return boost::phoenix::bind(tanh, x);  
  }
  
  template <typename T>
  auto Log(const T& x) -> decltype(boost::phoenix::bind(log, x)) {
    return boost::phoenix::bind(log, x);  
  }
  
  float logit(float x) {
    return 1.0 / (1.0 + exp(-x));
  }
  
  template <typename T>
  auto Logit(const T& x) -> decltype(boost::phoenix::bind(logit, x)) { 
    return boost::phoenix::bind(logit, x);  
  }
  
  
  float max(float x, float y) {
    return x > y ? x : y;
  }
  
  template <typename T1, typename T2>
  auto Max(const T1& x, const T2& y) -> decltype(boost::phoenix::bind(max, x, y)) { 
    return boost::phoenix::bind(max, x, y);  
  }
}