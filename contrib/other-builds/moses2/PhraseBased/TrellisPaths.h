/*
 * TrellisPaths.h
 *
 *  Created on: 16 Mar 2016
 *      Author: hieu
 */
#pragma once

#include <set>
#include "TrellisPath.h"

namespace Moses2 {

struct CompareTrellisPathCollection {
  bool operator()(const TrellisPath* pathA, const TrellisPath* pathB) const {
    return (pathA->GetFutureScore() > pathB->GetFutureScore());
  }
};

class TrellisPaths {
public:
  TrellisPaths();
  virtual ~TrellisPaths();

  size_t GetSize() const {
    return m_collection.size();
  }

  //! add a new entry into collection
  void Add(TrellisPath *trellisPath) {
    m_collection.insert(trellisPath);
  }

  TrellisPath *pop() {
    TrellisPath *top = *m_collection.begin();

    // Detach
    m_collection.erase(m_collection.begin());
    return top;
  }

protected:
  typedef std::multiset<TrellisPath*, CompareTrellisPathCollection> CollectionType;
  CollectionType m_collection;

};

} /* namespace Moses2 */

