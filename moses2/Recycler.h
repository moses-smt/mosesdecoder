/*
 * Recycler.h
 *
 *  Created on: 2 Jan 2016
 *      Author: hieu
 */
#pragma once

#include <cstddef>
#include <stack>
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
      T &obj = m_coll.top();
      m_coll.pop();
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
    m_coll.push(val);
  }

protected:
  // objects that have been give back to us
  std::stack<T> m_coll;
};

} /* namespace Moses2 */

