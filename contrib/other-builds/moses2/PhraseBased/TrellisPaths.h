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

protected:
  typedef std::multiset<TrellisPath*, CompareTrellisPathCollection> CollectionType;
  CollectionType m_collection;

};

} /* namespace Moses2 */

