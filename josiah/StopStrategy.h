#pragma once
#include <cstring>

namespace Josiah {
/**
 * Used to specify when the sampler should stop.
 **/
class StopStrategy {
public:
  virtual bool ShouldStop(std::size_t iterations) = 0;
  virtual ~StopStrategy() {}
};

/**
 * Simplest sampler stop strategy, just uses the number of iterations.
 **/
class CountStopStrategy : public virtual StopStrategy {
public:
  CountStopStrategy(std::size_t max): m_max(max) {}
  virtual bool ShouldStop(size_t iterations) {return iterations >= m_max;}
  virtual ~CountStopStrategy() {}
  
private:
  std::size_t m_max;
};

}

