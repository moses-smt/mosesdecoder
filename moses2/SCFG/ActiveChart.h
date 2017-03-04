#pragma once
#include <vector>
#include <iostream>
#include <boost/functional/hash/hash.hpp>
#include "../legacy/Range.h"
#include "../HypothesisColl.h"

namespace Moses2
{
class System;
class PhraseTable;

namespace SCFG
{
class InputPath;
class Word;

////////////////////////////////////////////////////////////////////////////
//! The range covered by each symbol in the source
//! Terminals will cover only 1 word, NT can cover multiple words
class SymbolBindElement
{
public:
  const SCFG::Word *word; // can be term or non-term

  const Moses2::Hypotheses *hypos; // NULL if terminal

  SymbolBindElement();
  SymbolBindElement(const Moses2::Range &range, const SCFG::Word &word, const Moses2::Hypotheses *hypos);

  const Range &GetRange() const {
    return *m_range;
  }

  bool operator==(const SymbolBindElement &compare) const {
    bool ret = hypos == compare.hypos
               && word == compare.word;
    return ret;
  }

  std::string Debug(const System &system) const;

protected:
  const Moses2::Range *m_range;

};

size_t hash_value(const SymbolBindElement &obj);

////////////////////////////////////////////////////////////////////////////
class SymbolBind
{
public:
  typedef Vector<SymbolBindElement> Coll;
  Coll coll;
  size_t numNT;

  SymbolBind(MemPool &pool);

  SymbolBind(MemPool &pool, const SymbolBind &copy)
    :coll(copy.coll)
    ,numNT(copy.numNT)
  {}

  size_t GetSize() const {
    return coll.size();
  }

  std::vector<const SymbolBindElement*> GetNTElements() const;

  void Add(const Range &range, const SCFG::Word &word, const Moses2::Hypotheses *hypos);

  bool operator==(const SymbolBind &compare) const {
    return coll == compare.coll;
  }

  std::string Debug(const System &system) const;

};

inline size_t hash_value(const SymbolBind &obj)
{
  return boost::hash_value(obj.coll);
}

////////////////////////////////////////////////////////////////////////////
class ActiveChartEntry
{
public:
  ActiveChartEntry(MemPool &pool);

  ActiveChartEntry(MemPool &pool, const ActiveChartEntry &prevEntry)
    :m_symbolBind(pool, prevEntry.GetSymbolBind()) {
    //symbolBinds = new (pool.Allocate<SymbolBind>()) SymbolBind(pool, *prevEntry.symbolBinds);
  }

  const SymbolBind &GetSymbolBind() const {
    return m_symbolBind;
  }

  virtual void AddSymbolBindElement(
    const Range &range,
    const SCFG::Word &word,
    const Moses2::Hypotheses *hypos,
    const PhraseTable &pt) {
    m_symbolBind.Add(range, word, hypos);
  }

protected:
  SymbolBind m_symbolBind;

};

////////////////////////////////////////////////////////////////////////////
class ActiveChart
{
public:
  ActiveChart(MemPool &pool);
  ~ActiveChart();

  Vector<ActiveChartEntry*> entries;
};

}
}

