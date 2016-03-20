/*
 * TrellisPaths.h
 *
 *  Created on: 16 Mar 2016
 *      Author: hieu
 */
#pragma once

#include <vector>
#include <queue>
#include "TrellisPath.h"

namespace Moses2 {

struct CompareTrellisPathCollection {
  bool operator()(const TrellisPath* pathA, const TrellisPath* pathB) const {
    return (pathA->GetFutureScore() < pathB->GetFutureScore());
  }
};

class TrellisPaths {
public:
  TrellisPaths();
  virtual ~TrellisPaths();

  bool empty() const {
    return m_collection.empty();
  }

  //! add a new entry into collection
  void Add(TrellisPath *trellisPath);

  TrellisPath *Get() {
    TrellisPath *top = m_collection.top();

    // Detach
    m_collection.pop();
    return top;
  }

protected:
  typedef std::priority_queue<TrellisPath*, std::vector<TrellisPath*>, CompareTrellisPathCollection> CollectionType;
  CollectionType m_collection;
};

} /* namespace Moses2 */

