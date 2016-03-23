// -*- c++ -*-
#include <vector>
#include <string>

#include "moses/FF/FFState.h"
#include "moses/Hypothesis.h"
#include "moses/Range.h"
#include "moses/TranslationOption.h"
#include "moses/Util.h"

#include "LexicalReordering.h"
#include "LexicalReorderingState.h"
#include "ReorderingStack.h"
#include "HReorderingForwardState.h"
#include "HReorderingBackwardState.h"
#include "PhraseBasedReorderingState.h"

namespace Moses
{

bool
IsMonotonicStep(Range  const& prev, // words range of last source phrase
                Range  const& cur,  // words range of current source phrase
                Bitmap const& cov)  // coverage bitmap
{
  size_t e = prev.GetEndPos() + 1;
  size_t s = cur.GetStartPos();
  return (s == e || (s >= e && !cov.GetValue(e)));
}

bool
IsSwap(Range const& prev, Range const& cur, Bitmap const& cov)
{
  size_t s = prev.GetStartPos();
  size_t e = cur.GetEndPos();
  return (e+1 == s || (e < s && !cov.GetValue(s-1)));
}

size_t
LRModel::
GetNumberOfTypes() const
{
  return ((m_modelType == MSD)  ? 3 :
          (m_modelType == MSLR) ? 4 : 2);
}

size_t
LRModel::
GetNumScoreComponents() const
{
  size_t score_per_dir = m_collapseScores ? 1 : GetNumberOfTypes();
  return ((m_direction == Bidirectional)
          ? 2 * score_per_dir + m_additionalScoreComponents
          : score_per_dir + m_additionalScoreComponents);
}

void
LRModel::
ConfigureSparse(const std::map<std::string,std::string>& sparseArgs,
                const LexicalReordering* producer)
{
  if (sparseArgs.size()) {
    m_sparse.reset(new SparseReordering(sparseArgs, producer));
  }
}

void
LRModel::
SetAdditionalScoreComponents(size_t number)
{
  m_additionalScoreComponents = number;
}

/// return orientation for the first phrase
LRModel::ReorderingType
LRModel::
GetOrientation(Range const& cur) const
{
  UTIL_THROW_IF2(m_modelType == None, "Reordering Model Type is None");
  return ((m_modelType == LeftRight) ? R :
          (cur.GetStartPos() == 0) ? M  :
          (m_modelType == MSD)     ? D  :
          (m_modelType == MSLR)    ? DR : NM);
}

LRModel::ReorderingType
LRModel::
GetOrientation(Range const& prev, Range const& cur) const
{
  UTIL_THROW_IF2(m_modelType == None, "No reordering model type specified");
  return ((m_modelType == LeftRight)
          ? prev.GetEndPos() <= cur.GetStartPos() ? R : L
        : (cur.GetStartPos() == prev.GetEndPos() + 1) ? M
          : (m_modelType == Monotonic) ? NM
          : (prev.GetStartPos() ==  cur.GetEndPos() + 1) ? S
          : (m_modelType == MSD) ? D
          : (cur.GetStartPos() > prev.GetEndPos()) ? DR : DL);
}

LRModel::ReorderingType
LRModel::
GetOrientation(int const reoDistance) const
{
  // this one is for HierarchicalReorderingBackwardState
  return ((m_modelType == LeftRight)
          ? (reoDistance >= 1) ? R : L
        : (reoDistance == 1) ? M
          : (m_modelType == Monotonic) ? NM
          : (reoDistance == -1)  ? S
          : (m_modelType == MSD) ? D
          : (reoDistance  >  1) ? DR : DL);
}

LRModel::ReorderingType
LRModel::
GetOrientation(Range const& prev, Range const& cur,
               Bitmap const& cov) const
{
  return ((m_modelType == LeftRight)
          ? cur.GetStartPos() > prev.GetEndPos() ? R : L
        : IsMonotonicStep(prev,cur,cov) ? M
          : (m_modelType == Monotonic) ? NM
          : IsSwap(prev,cur,cov) ? S
          : (m_modelType == MSD) ? D
          : cur.GetStartPos() > prev.GetEndPos() ? DR : DL);
}

LRModel::
LRModel(const std::string &modelType)
  : m_modelString(modelType)
  , m_scoreProducer(NULL)
  , m_modelType(None)
  , m_phraseBased(true)
  , m_collapseScores(false)
  , m_direction(Backward)
  , m_additionalScoreComponents(0)
{
  std::vector<std::string> config = Tokenize<std::string>(modelType, "-");

  for (size_t i=0; i<config.size(); ++i) {
    if      (config[i] == "hier")   {
      m_phraseBased = false;
    } else if (config[i] == "phrase") {
      m_phraseBased = true;
    } else if (config[i] == "wbe")    {
      m_phraseBased = true;
    }
    // no word-based decoding available, fall-back to phrase-based
    // This is the old lexical reordering model combination of moses

    else if (config[i] == "msd")          {
      m_modelType = MSD;
    } else if (config[i] == "mslr")         {
      m_modelType = MSLR;
    } else if (config[i] == "monotonicity") {
      m_modelType = Monotonic;
    } else if (config[i] == "leftright")    {
      m_modelType = LeftRight;
    }

    // unidirectional is deprecated, use backward instead
    else if (config[i] == "unidirectional") {
      m_direction = Backward;
    } else if (config[i] == "backward")       {
      m_direction = Backward;
    } else if (config[i] == "forward")        {
      m_direction = Forward;
    } else if (config[i] == "bidirectional")  {
      m_direction = Bidirectional;
    }

    else if (config[i] == "f")  {
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
    std::cerr
        << "You need to specify the type of the reordering model "
        << "(msd, monotonicity,...)" << std::endl;
    exit(1);
  }
}

LRState *
LRModel::
CreateLRState(const InputType &input) const
{
  LRState *bwd = NULL, *fwd = NULL;
  size_t offset = 0;

  switch(m_direction) {
  case Backward:
  case Bidirectional:
    if (m_phraseBased)
      bwd = new PhraseBasedReorderingState(*this, Backward, offset);
    else
      bwd = new HReorderingBackwardState(*this, offset);
    offset += m_collapseScores ? 1 : GetNumberOfTypes();
    if (m_direction == Backward) return bwd; // else fall through
  case Forward:
    if (m_phraseBased)
      fwd = new PhraseBasedReorderingState(*this, Forward, offset);
    else
      fwd = new HReorderingForwardState(*this, input.GetSize(), offset);
    offset += m_collapseScores ? 1 : GetNumberOfTypes();
    if (m_direction == Forward) return fwd;
  }
  return new BidirectionalReorderingState(*this, bwd, fwd, 0);
}


void
LRState::
CopyScores(ScoreComponentCollection*  accum,
           const TranslationOption &topt,
           const InputType& input,
           ReorderingType reoType) const
{
  // don't call this on a bidirectional object
  UTIL_THROW_IF2(m_direction != LRModel::Backward &&
                 m_direction != LRModel::Forward,
                 "Unknown direction: " << m_direction);

  TranslationOption const* relevantOpt = ((m_direction == LRModel::Backward)
                                          ? &topt : m_prevOption);

  LexicalReordering* producer = m_configuration.GetScoreProducer();
  Scores const* cached = relevantOpt->GetLexReorderingScores(producer);

  // The approach here is bizarre! Why create a whole vector and do
  // vector addition (acumm->PlusEquals) to update a single value? - UG
  size_t off_remote = m_offset + reoType;
  size_t off_local  = m_configuration.CollapseScores() ? m_offset : off_remote;

  UTIL_THROW_IF2(off_local >= producer->GetNumScoreComponents(),
                 "offset out of vector bounds!");

  // look up applicable score from vectore of scores
  if(cached) {
    UTIL_THROW_IF2(off_remote >= cached->size(), "offset out of vector bounds!");
    Scores scores(producer->GetNumScoreComponents(),0);
    scores[off_local ] = (*cached)[off_remote];
    accum->PlusEquals(producer, scores);
  }

  // else: use default scores (if specified)
  else if (producer->GetHaveDefaultScores()) {
    Scores scores(producer->GetNumScoreComponents(),0);
    scores[off_local] = producer->GetDefaultScore(off_remote);
    accum->PlusEquals(m_configuration.GetScoreProducer(), scores);
  }
  // note: if no default score, no cost

  const SparseReordering* sparse = m_configuration.GetSparseReordering();
  if (sparse) sparse->CopyScores(*relevantOpt, m_prevOption, input, reoType,
                                   m_direction, accum);
}


int
LRState::
ComparePrevScores(const TranslationOption *other) const
{
  LexicalReordering* producer = m_configuration.GetScoreProducer();
  const Scores* myScores = m_prevOption->GetLexReorderingScores(producer);
  const Scores* yrScores = other->GetLexReorderingScores(producer);

  if(myScores == yrScores) return 0;

  // The pointers are NULL if a phrase pair isn't found in the reordering table.
  if(yrScores == NULL) return -1;
  if(myScores == NULL) return  1;

  size_t stop = m_offset + m_configuration.GetNumberOfTypes();
  for(size_t i = m_offset; i < stop; i++) {
    if((*myScores)[i] < (*yrScores)[i]) return -1;
    if((*myScores)[i] > (*yrScores)[i]) return  1;
  }
  return 0;
}

///////////////////////////
//BidirectionalReorderingState

size_t BidirectionalReorderingState::hash() const
{
  size_t ret = m_backward->hash();
  boost::hash_combine(ret, m_forward->hash());
  return ret;
}

bool BidirectionalReorderingState::operator==(const FFState& o) const
{
  if (&o == this) return 0;

  BidirectionalReorderingState const &other
  = static_cast<BidirectionalReorderingState const&>(o);

  bool ret = (*m_backward == *other.m_backward) && (*m_forward == *other.m_forward);
  return ret;
}

LRState*
BidirectionalReorderingState::
Expand(const TranslationOption& topt, const InputType& input,
       ScoreComponentCollection* scores) const
{
  LRState *newbwd = m_backward->Expand(topt,input, scores);
  LRState *newfwd = m_forward->Expand(topt, input, scores);
  return new BidirectionalReorderingState(m_configuration, newbwd, newfwd, m_offset);
}


}

