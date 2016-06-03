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
  friend std::ostream& operator<<(std::ostream &out, const SCFG::Stack &obj);

public:
  typedef boost::unordered_map<SCFG::Word, Moses2::HypothesisColl*> Coll;

  Stack(const Manager &mgr);
  virtual ~Stack();

  Coll &GetColl()
  {  return m_coll; }

  const Coll &GetColl() const
  {  return m_coll; }

  const Moses2::HypothesisColl *GetColl(SCFG::Word &nt) const;

  size_t GetSize() const;

  void Add(SCFG::Hypothesis *hypo, Recycler<HypothesisBase*> &hypoRecycle,
      ArcLists &arcLists);

  const Hypothesis *GetBestHypo(
      const Manager &mgr,
      ArcLists &arcLists) const;

protected:
  const Manager &m_mgr;
  Coll m_coll;

  Moses2::HypothesisColl &GetMiniStack(const SCFG::Word &key);

};

}

}

