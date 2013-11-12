
#pragma once

#include <boost/unordered_set.hpp>
#include "Search/Hypothesis.h"

class InputPath;
class TargetPhrases;
class Stacks;

class Stack
{
protected:
  typedef boost::unordered_set<const Hypothesis*,
          HypothesisHasher,
          HypothesisEqual> Coll;
  Coll m_coll;
  size_t m_maxHypoStackSize;
  Stacks *m_stacks;
  
  std::pair<Coll::iterator, bool> Add(const Hypothesis *hypo);

  void Remove(Coll::iterator &iter);
  void SortHypotheses(size_t newSize, std::vector<const Hypothesis*> &out);
  void Extend(const Hypothesis &hypo, const std::vector<InputPath*> &queue);
  void Extend(const Hypothesis &hypo, const InputPath &path);
  void Extend(const Hypothesis &hypo, const TargetPhrases &tpColl, const WordsRange &range);
  void PruneToSize(size_t newSize);

public:
  typedef Coll::iterator iterator;
  typedef Coll::const_iterator const_iterator;
  //! iterators
  const_iterator begin() const {
    return m_coll.begin();
  }
  const_iterator end() const {
    return m_coll.end();
  }

  Stack();
  virtual ~Stack();

  size_t GetSize() const {
    return m_coll.size();
  }

  const Hypothesis *GetHypothesis() const;

  bool AddPrune(Hypothesis *hypo);
  void PruneToSize();

  void Search(const std::vector<InputPath*> &queue);

  void SetContainer(Stacks &stacks) {
    m_stacks = &stacks;
  }

  std::string Debug() const;

};

