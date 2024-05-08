/*
 * Recycler.h
 *
 *  Created on: 2 Jan 2016
 *      Author: hieu
 */
#pragma once

#include <cstddef>
#include <deque>
#include <vector>

namespace Moses2
{

template<typename T>
class Recycler
{
public:
  Recycler() {
  }
  
  virtual ~Recycler() {
  }

  T Get() {
    if (!m_coll.empty()) {
      T &obj = m_coll.back();
      m_coll.pop_back();
      return obj;
    } else {
      return NULL;
    }
  }

  void Clear() {
    m_coll.clear();
  }

  // call this for existing object to put back into queue for reuse
  void Recycle(const T& val) {
    m_coll.push_back(val);
  }

protected:
  // objects that have been give back to us
  std::deque<T> m_coll;
};

} /* namespace Moses2 */

