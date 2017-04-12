/*
 * LRModel.cpp
 *
 *  Created on: 23 Mar 2016
 *      Author: hieu
 */

#include "LRModel.h"
#include "../../legacy/Util2.h"
#include "../../legacy/Range.h"
#include "../../legacy/Bitmap.h"
#include "../../MemPool.h"
#include "util/exception.hh"
#include "PhraseBasedReorderingState.h"
#include "BidirectionalReorderingState.h"
#include "HReorderingBackwardState.h"
#include "HReorderingForwardState.h"

using namespace std;

namespace Moses2
{

bool IsMonotonicStep(Range const& prev, // words range of last source phrase
                     Range const& cur,  // words range of current source phrase
                     Bitmap const& cov)  // coverage bitmap
{
  size_t e = prev.GetEndPos() + 1;
  size_t s = cur.GetStartPos();
  return (s == e || (s >= e && !cov.GetValue(e)));
}

bool IsSwap(Range const& prev, Range const& cur, Bitmap const& cov)
{
  size_t s = prev.GetStartPos();
  size_t e = cur.GetEndPos();
  return (e + 1 == s || (e < s && !cov.GetValue(s - 1)));
}

LRModel::LRModel(const std::string &modelType, LexicalReordering &ff) :
  m_modelType(None), m_phraseBased(true), m_collapseScores(false), m_direction(
    Backward), m_scoreProducer(&ff)
{
  std::vector<std::string> config = Tokenize(modelType, "-");

  for (size_t i = 0; i < config.size(); ++i) {
    if (config[i] == "hier") {
      m_phraseBased = false;
    } else if (config[i] == "phrase") {
      m_phraseBased = true;
    } else if (config[i] == "wbe") {
      m_phraseBased = true;
    }
    // no word-based decoding available, fall-back to phrase-based
    // This is the old lexical reordering model combination of moses

    else if (config[i] == "msd") {
      m_modelType = MSD;
    } else if (config[i] == "mslr") {
      m_modelType = MSLR;
    } else if (config[i] == "monotonicity") {
      m_modelType = Monotonic;
    } else if (config[i] == "leftright") {
      m_modelType = LeftRight;
    }

    // unidirectional is deprecated, use backward instead
    else if (config[i] == "unidirectional") {
      m_direction = Backward;
    } else if (config[i] == "backward") {
      m_direction = Backward;
    } else if (config[i] == "forward") {
      m_direction = Forward;
    } else if (config[i] == "bidirectional") {
      m_direction = Bidirectional;
    }

    else if (config[i] == "f") {
      m_condition = F;
    } else if (config[i] == "fe") {
      m_condition = FE;
    }

    else if (config[i] == "collapseff") {
      m_collapseScores = true;
    } else if (config[i] == "allff") {
      m_collapseScores = false;
    } else {
      std::cerr
          << "Illegal part in the lexical reordering configuration string: "
          << config[i] << std::endl;
      exit(1);
    }
  }

  if (m_modelType == None) {
    std::cerr << "You need to specify the type of the reordering model "
              << "(msd, monotonicity,...)" << std::endl;
    exit(1);
  }

}

LRModel::~LRModel()
{
  // TODO Auto-generated destructor stub
}

size_t LRModel::GetNumberOfTypes() const
{
  return ((m_modelType == MSD) ? 3 : (m_modelType == MSLR) ? 4 : 2);
}

/// return orientation for the first phrase
LRModel::ReorderingType LRModel::GetOrientation(Range const& cur) const
{
  UTIL_THROW_IF2(m_modelType == None, "Reordering Model Type is None");
  return ((m_modelType == LeftRight) ? R : (cur.GetStartPos() == 0) ? M :
          (m_modelType == MSD) ? D : (m_modelType == MSLR) ? DR : NM);
}

LRModel::ReorderingType LRModel::GetOrientation(Range const& prev,
    Range const& cur) const
{
  UTIL_THROW_IF2(m_modelType == None, "No reordering model type specified");
  return (
           (m_modelType == LeftRight) ? prev.GetEndPos() <= cur.GetStartPos() ? R : L
         : (cur.GetStartPos() == prev.GetEndPos() + 1) ? M :
           (m_modelType == Monotonic) ? NM :
           (prev.GetStartPos() == cur.GetEndPos() + 1) ? S :
           (m_modelType == MSD) ? D :
           (cur.GetStartPos() > prev.GetEndPos()) ? DR : DL);
}

LRModel::ReorderingType LRModel::GetOrientation(int const reoDistance) const
{
  // this one is for HierarchicalReorderingBackwardState
  return ((m_modelType == LeftRight) ? (reoDistance >= 1) ? R : L
        : (reoDistance == 1) ? M : (m_modelType == Monotonic) ? NM :
          (reoDistance == -1) ? S : (m_modelType == MSD) ? D :
          (reoDistance > 1) ? DR : DL);
}

LRState *LRModel::CreateLRState(MemPool &pool) const
{
  LRState *bwd = NULL, *fwd = NULL;
  size_t offset = 0;

  switch (m_direction) {
  case Backward:
  case Bidirectional:
    if (m_phraseBased) {
      bwd =
        new (pool.Allocate<PhraseBasedReorderingState>()) PhraseBasedReorderingState(
        *this, Backward, offset);
      //cerr << "bwd=" << bwd << bwd->ToString() << endl;
    } else {
      bwd =
        new (pool.Allocate<HReorderingBackwardState>()) HReorderingBackwardState(
        pool, *this, offset);
    }
    offset += m_collapseScores ? 1 : GetNumberOfTypes();
    if (m_direction == Backward) return bwd; // else fall through
  case Forward:
    if (m_phraseBased) {
      fwd =
        new (pool.Allocate<PhraseBasedReorderingState>()) PhraseBasedReorderingState(
        *this, Forward, offset);
      //cerr << "fwd=" << fwd << fwd->ToString() << endl;
    } else {
      fwd =
        new (pool.Allocate<HReorderingForwardState>()) HReorderingForwardState(
        *this, offset);
    }
    offset += m_collapseScores ? 1 : GetNumberOfTypes();
    if (m_direction == Forward) return fwd;
  }

  //cerr << "LRStates:" << *bwd << endl << *fwd << endl;
  BidirectionalReorderingState *ret =
    new (pool.Allocate<BidirectionalReorderingState>()) BidirectionalReorderingState(
    *this, bwd, fwd, 0);
  return ret;
}

LRModel::ReorderingType LRModel::GetOrientation(Range const& prev,
    Range const& cur, Bitmap const& cov) const
{
  return (
           (m_modelType == LeftRight) ? cur.GetStartPos() > prev.GetEndPos() ? R : L
         : IsMonotonicStep(prev, cur, cov) ? M : (m_modelType == Monotonic) ? NM :
           IsSwap(prev, cur, cov) ? S : (m_modelType == MSD) ? D :
           cur.GetStartPos() > prev.GetEndPos() ? DR : DL);
}

} /* namespace Moses2 */
