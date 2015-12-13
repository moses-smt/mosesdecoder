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

// ===========================================================================
// PHRASE BASED REORDERING STATE
// ===========================================================================
bool PhraseBasedReorderingState::m_useFirstBackwardScore = true;

PhraseBasedReorderingState::
PhraseBasedReorderingState(const PhraseBasedReorderingState *prev,
                           const TranslationOption &topt)
  : LRState(prev, topt)
  , m_prevRange(topt.GetSourceWordsRange())
  , m_first(false)
{ }


PhraseBasedReorderingState::
PhraseBasedReorderingState(const LRModel &config,
                           LRModel::Direction dir, size_t offset)
  : LRState(config, dir, offset)
  , m_prevRange(NOT_FOUND,NOT_FOUND)
  , m_first(true)
{ }


size_t PhraseBasedReorderingState::hash() const
{
  size_t ret;
  ret = hash_value(m_prevRange);
  boost::hash_combine(ret, m_direction);

  return ret;
}

bool PhraseBasedReorderingState::operator==(const FFState& o) const
{
  if (&o == this) return true;

  const PhraseBasedReorderingState &other = static_cast<const PhraseBasedReorderingState&>(o);
  if (m_prevRange == other.m_prevRange) {
    if (m_direction == LRModel::Forward) {
      int compareScore = ComparePrevScores(other.m_prevOption);
      return compareScore == 0;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

LRState*
PhraseBasedReorderingState::
Expand(const TranslationOption& topt, const InputType& input,
       ScoreComponentCollection* scores) const
{
  // const LRModel::ModelType modelType = m_configuration.GetModelType();

  if ((m_direction != LRModel::Forward && m_useFirstBackwardScore) || !m_first) {
    LRModel const& lrmodel = m_configuration;
    Range const cur = topt.GetSourceWordsRange();
    LRModel::ReorderingType reoType = (m_first ? lrmodel.GetOrientation(cur)
                                       : lrmodel.GetOrientation(m_prevRange,cur));
    CopyScores(scores, topt, input, reoType);
  }
  return new PhraseBasedReorderingState(this, topt);
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

///////////////////////////
//HierarchicalReorderingBackwardState

HReorderingBackwardState::
HReorderingBackwardState(const HReorderingBackwardState *prev,
                         const TranslationOption &topt,
                         ReorderingStack reoStack)
  : LRState(prev, topt),  m_reoStack(reoStack)
{ }

HReorderingBackwardState::
HReorderingBackwardState(const LRModel &config, size_t offset)
  : LRState(config, LRModel::Backward, offset)
{ }

size_t HReorderingBackwardState::hash() const
{
  size_t ret = m_reoStack.hash();
  return ret;
}

bool HReorderingBackwardState::operator==(const FFState& o) const
{
  const HReorderingBackwardState& other
  = static_cast<const HReorderingBackwardState&>(o);
  bool ret = m_reoStack == other.m_reoStack;
  return ret;
}

LRState*
HReorderingBackwardState::
Expand(const TranslationOption& topt, const InputType& input,
       ScoreComponentCollection*  scores) const
{
  HReorderingBackwardState* nextState;
  nextState = new HReorderingBackwardState(this, topt, m_reoStack);
  Range swrange = topt.GetSourceWordsRange();
  int reoDistance = nextState->m_reoStack.ShiftReduce(swrange);
  ReorderingType reoType = m_configuration.GetOrientation(reoDistance);
  CopyScores(scores, topt, input, reoType);
  return nextState;
}

///////////////////////////
//HReorderingForwardState

HReorderingForwardState::
HReorderingForwardState(const LRModel &config,
                        size_t size, size_t offset)
  : LRState(config, LRModel::Forward, offset)
  , m_first(true)
  , m_prevRange(NOT_FOUND,NOT_FOUND)
  , m_coverage(size)
{ }

HReorderingForwardState::
HReorderingForwardState(const HReorderingForwardState *prev,
                        const TranslationOption &topt)
  : LRState(prev, topt)
  , m_first(false)
  , m_prevRange(topt.GetSourceWordsRange())
  , m_coverage(prev->m_coverage, topt.GetSourceWordsRange())
{
}

size_t HReorderingForwardState::hash() const
{
  size_t ret;
  ret = hash_value(m_prevRange);
  return ret;
}

bool HReorderingForwardState::operator==(const FFState& o) const
{
  if (&o == this) return true;

  HReorderingForwardState const& other
  = static_cast<HReorderingForwardState const&>(o);

  int compareScores = ((m_prevRange == other.m_prevRange)
                       ? ComparePrevScores(other.m_prevOption)
                       : (m_prevRange < other.m_prevRange) ? -1 : 1);
  return compareScores == 0;
}

// For compatibility with the phrase-based reordering model, scoring is one
// step delayed.
// The forward model takes determines orientations heuristically as follows:
//  mono:   if the next phrase comes after the conditioning phrase and
//          - there is a gap to the right of the conditioning phrase, or
//          - the next phrase immediately follows it
//  swap:   if the next phrase goes before the conditioning phrase and
//          - there is a gap to the left of the conditioning phrase, or
//          - the next phrase immediately precedes it
//  dright: if the next phrase follows the conditioning phrase and other
//          stuff comes in between
//  dleft:  if the next phrase precedes the conditioning phrase and other
//          stuff comes in between

LRState*
HReorderingForwardState::
Expand(TranslationOption const& topt, InputType const& input,
       ScoreComponentCollection* scores) const
{
  const Range cur = topt.GetSourceWordsRange();
  // keep track of the current coverage ourselves so we don't need the hypothesis
  Bitmap cov(m_coverage, cur);
  if (!m_first) {
    LRModel::ReorderingType reoType;
    reoType = m_configuration.GetOrientation(m_prevRange,cur,cov);
    CopyScores(scores, topt, input, reoType);
  }
  return new HReorderingForwardState(this, topt);
}
}

