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
  Recycler() :
    m_currInd(0) {
  }
  virtual ~Recycler() {
  }

  T Get() {
    if (!m_coll.empty()) {
      T &obj = m_coll.back();
      m_coll.pop_back();
      return obj;
    } else if (m_currInd) {
      --m_currInd;
      T &obj = m_all[m_currInd];
      return obj;
    } else {
      return NULL;
    }
  }

  void Clear() {
    m_coll.clear();
    m_currInd = m_all.size();
  }

  // call this for new objects when u 1st create it. It is assumed the object will be used right away
  void Keep(const T& val) {
    m_all.push_back(val);
  }

  // call this for existing object to put back into queue for reuse
  void Recycle(const T& val) {
    m_coll.push_back(val);
  }

protected:
  // all objects we're looking after
  std::vector<T> m_all;

  // pointer to the object that's just been given out.
  // to give out another obj, must decrement THEN give out
  size_t m_currInd;

  // objects that have been give back to us
  std::deque<T> m_coll;
};

} /* namespace Moses2 */

