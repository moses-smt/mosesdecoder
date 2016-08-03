/*
 * TrellisPaths.h
 *
 *  Created on: 16 Mar 2016
 *      Author: hieu
 */
#pragma once

#include <vector>
#include <queue>
#include "PhraseBased/TrellisPath.h"

namespace Moses2
{

template<typename T>
struct CompareTrellisPath
{
  bool operator()(const T* pathA, const T* pathB) const
  {
    return (pathA->GetFutureScore() < pathB->GetFutureScore());
  }
};

template<typename T>
class TrellisPaths
{
public:
  TrellisPaths() {}

  virtual ~TrellisPaths()
  {
    while (!empty()) {
      T *path = Get();
      delete path;
    }
  }

  bool empty() const
  {
    return m_collection.empty();
  }

  //! add a new entry into collection
  void Add(T *trellisPath)
  {
    m_collection.push(trellisPath);
  }

  T *Get()
  {
    T *top = m_collection.top();

    // Detach
    m_collection.pop();
    return top;
  }

protected:
  typedef std::priority_queue<T*, std::vector<T*>,
      CompareTrellisPath<T> > CollectionType;
  CollectionType m_collection;
};

} /* namespace Moses2 */

