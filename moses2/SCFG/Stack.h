#pragma once
#include <boost/unordered_map.hpp>
#include "../HypothesisColl.h"
#include "../Recycler.h"
#include "Word.h"

namespace Moses2
{
class HypothesisBase;
class ArcLists;

namespace SCFG
{
class Hypothesis;
class Manager;

class Stack
{
public:
  typedef boost::unordered_map<SCFG::Word, Moses2::HypothesisColl*> Coll;

  Stack(const Manager &mgr);
  virtual ~Stack();

  const Coll &GetColl() const {
    return m_coll;
  }

  const Moses2::HypothesisColl *GetColl(const SCFG::Word &nt) const;

  size_t GetSize() const;

  void Add(SCFG::Hypothesis *hypo, Recycler<HypothesisBase*> &hypoRecycle,
           ArcLists &arcLists);

  const Hypothesis *GetBestHypo() const;

  std::string Debug(const System &system) const;

protected:
  const Manager &m_mgr;
  Coll m_coll;

  Moses2::HypothesisColl &GetColl(const SCFG::Word &nt);

};

}

}

