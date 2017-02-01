/*
 * ArcList.h
 *
 *  Created on: 26 Oct 2015
 *      Author: hieu
 */
#pragma once
#include <vector>
#include <boost/unordered_map.hpp>

namespace Moses2
{
class System;

class HypothesisBase;

typedef std::vector<const HypothesisBase*> ArcList;

class ArcLists
{
public:
  ArcLists();
  virtual ~ArcLists();

  void AddArc(bool added, const HypothesisBase *currHypo,
              const HypothesisBase *otherHypo);
  void Sort();
  void Delete(const HypothesisBase *hypo);

  const ArcList &GetArcList(const HypothesisBase *hypo) const;

  std::string Debug(const System &system) const;
protected:
  typedef boost::unordered_map<const HypothesisBase*, ArcList*> Coll;
  Coll m_coll;

  ArcList &GetArcList(const HypothesisBase *hypo);
  ArcList &GetAndDetachArcList(const HypothesisBase *hypo);

};

}

