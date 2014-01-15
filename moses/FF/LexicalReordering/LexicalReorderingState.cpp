
#include <vector>
#include <string>

#include "moses/FF/FFState.h"
#include "moses/Hypothesis.h"
#include "moses/WordsRange.h"
#include "moses/ReorderingStack.h"
#include "moses/TranslationOption.h"

#include "LexicalReordering.h"
#include "LexicalReorderingState.h"

namespace Moses
{

size_t LexicalReorderingConfiguration::GetNumberOfTypes() const
{
  switch (m_modelType) {
  case LexicalReorderingConfiguration::MSD:
    return 3;
    break;
  case LexicalReorderingConfiguration::MSLR:
    return 4;
    break;
  default:
    return 2;
  }
}

size_t LexicalReorderingConfiguration::GetNumScoreComponents() const
{
  size_t score_per_dir = m_collapseScores ? 1 : GetNumberOfTypes();
  if (m_direction == Bidirectional) {
    return 2 * score_per_dir + m_additionalScoreComponents;
  } else {
    return score_per_dir + m_additionalScoreComponents;
  }
}

void LexicalReorderingConfiguration::SetAdditionalScoreComponents(size_t number)
{
  m_additionalScoreComponents = number;
}

LexicalReorderingConfiguration::LexicalReorderingConfiguration(const std::string &modelType)
  : m_modelString(modelType), m_scoreProducer(NULL), m_modelType(None), m_phraseBased(true), m_collapseScores(false), m_direction(Backward), m_additionalScoreComponents(0)
{
  std::vector<std::string> config = Tokenize<std::string>(modelType, "-");

  for (size_t i=0; i<config.size(); ++i) {
    if (config[i] == "hier") {
      m_phraseBased = false;
    } else if (config[i] == "phrase") {
      m_phraseBased = true;
    } else if (config[i] == "wbe") {
      m_phraseBased = true;
      // no word-based decoding available, fall-back to phrase-based
      // This is the old lexical reordering model combination of moses
    } else if (config[i] == "msd") {
      m_modelType = MSD;
    } else if (config[i] == "mslr") {
      m_modelType = MSLR;
    } else if (config[i] == "monotonicity") {
      m_modelType = Monotonic;
    } else if (config[i] == "leftright") {
      m_modelType = LeftRight;
    } else if (config[i] == "backward" || config[i] == "unidirectional") {
      // note: unidirectional is deprecated, use backward instead
      m_direction = Backward;
    } else if (config[i] == "forward") {
      m_direction = Forward;
    } else if (config[i] == "bidirectional") {
      m_direction = Bidirectional;
    } else if (config[i] == "f") {
      m_condition = F;
    } else if (config[i] == "fe") {
      m_condition = FE;
    } else if (config[i] == "collapseff") {
      m_collapseScores = true;
    } else if (config[i] == "allff") {
      m_collapseScores = false;
    } else {
      UserMessage::Add("Illegal part in the lexical reordering configuration string: "+config[i]);
      exit(1);
    }
  }

  if (m_modelType == None) {
    UserMessage::Add("You need to specify the type of the reordering model (msd, monotonicity,...)");
    exit(1);
  }
}

LexicalReorderingState *LexicalReorderingConfiguration::CreateLexicalReorderingState(const InputType &input) const
{
  LexicalReorderingState *bwd = NULL, *fwd = NULL;
  size_t offset = 0;

  switch(m_direction) {
  case Backward:
  case Bidirectional:
    if (m_phraseBased) {  //Same for forward and backward
      bwd = new PhraseBasedReorderingState(*this, LexicalReorderingConfiguration::Backward, offset);
    } else {
      bwd = new HierarchicalReorderingBackwardState(*this, offset);
    }
    offset += m_collapseScores ? 1 : GetNumberOfTypes();
    if (m_direction == Backward)
      return bwd; // else fall through
  case Forward:
    if (m_phraseBased) {  //Same for forward and backward
      fwd = new PhraseBasedReorderingState(*this, LexicalReorderingConfiguration::Forward, offset);
    } else {
      fwd = new HierarchicalReorderingForwardState(*this, input.GetSize(), offset);
    }
    offset += m_collapseScores ? 1 : GetNumberOfTypes();
    if (m_direction == Forward)
      return fwd;
  }

  return new BidirectionalReorderingState(*this, bwd, fwd, 0);
}

void LexicalReorderingState::CopyScores(Scores& scores, const TranslationOption &topt, ReorderingType reoType) const
{
  // don't call this on a bidirectional object
  UTIL_THROW_IF2(m_direction != LexicalReorderingConfiguration::Backward && m_direction != LexicalReorderingConfiguration::Forward,
		  "Unknown direction: " << m_direction);
  const Scores *cachedScores = (m_direction == LexicalReorderingConfiguration::Backward) ?
                               topt.GetLexReorderingScores(m_configuration.GetScoreProducer()) : m_prevScore;

  // No scores available. TODO: Using a good prior distribution would be nicer.
  if(cachedScores == NULL)
    return;

  const Scores &scoreSet = *cachedScores;
  if(m_configuration.CollapseScores())
    scores[m_offset] = scoreSet[m_offset + reoType];
  else {
    std::fill(scores.begin() + m_offset, scores.begin() + m_offset + m_configuration.GetNumberOfTypes(), 0);
    scores[m_offset + reoType] = scoreSet[m_offset + reoType];
  }
}

void LexicalReorderingState::ClearScores(Scores& scores) const
{
  if(m_configuration.CollapseScores())
    scores[m_offset] = 0;
  else
    std::fill(scores.begin() + m_offset, scores.begin() + m_offset + m_configuration.GetNumberOfTypes(), 0);
}

int LexicalReorderingState::ComparePrevScores(const Scores *other) const
{
  if(m_prevScore == other)
    return 0;

  // The pointers are NULL if a phrase pair isn't found in the reordering table.
  if(other == NULL)
    return -1;
  if(m_prevScore == NULL)
    return 1;

  const Scores &my = *m_prevScore;
  const Scores &their = *other;
  for(size_t i = m_offset; i < m_offset + m_configuration.GetNumberOfTypes(); i++)
    if(my[i] < their[i])
      return -1;
    else if(my[i] > their[i])
      return 1;

  return 0;
}

bool PhraseBasedReorderingState::m_useFirstBackwardScore = true;

PhraseBasedReorderingState::PhraseBasedReorderingState(const PhraseBasedReorderingState *prev, const TranslationOption &topt)
  : LexicalReorderingState(prev, topt), m_prevRange(topt.GetSourceWordsRange()), m_first(false) {}


PhraseBasedReorderingState::PhraseBasedReorderingState(const LexicalReorderingConfiguration &config,
    LexicalReorderingConfiguration::Direction dir, size_t offset)
  : LexicalReorderingState(config, dir, offset), m_prevRange(NOT_FOUND,NOT_FOUND), m_first(true) {}


int PhraseBasedReorderingState::Compare(const FFState& o) const
{
  if (&o == this)
    return 0;

  const PhraseBasedReorderingState* other = dynamic_cast<const PhraseBasedReorderingState*>(&o);
  UTIL_THROW_IF2(other == NULL, "Wrong state type");
  if (m_prevRange == other->m_prevRange) {
    if (m_direction == LexicalReorderingConfiguration::Forward) {
      return ComparePrevScores(other->m_prevScore);
    } else {
      return 0;
    }
  } else if (m_prevRange < other->m_prevRange) {
    return -1;
  }
  return 1;
}

LexicalReorderingState* PhraseBasedReorderingState::Expand(const TranslationOption& topt, Scores& scores) const
{
  ReorderingType reoType;
  const WordsRange currWordsRange = topt.GetSourceWordsRange();
  const LexicalReorderingConfiguration::ModelType modelType = m_configuration.GetModelType();

  if (m_direction == LexicalReorderingConfiguration::Forward && m_first) {
    ClearScores(scores);
  } else {
    if (!m_first || m_useFirstBackwardScore) {
      if (modelType == LexicalReorderingConfiguration::MSD) {
        reoType = GetOrientationTypeMSD(currWordsRange);
      } else if (modelType == LexicalReorderingConfiguration::MSLR) {
        reoType = GetOrientationTypeMSLR(currWordsRange);
      } else if (modelType == LexicalReorderingConfiguration::Monotonic) {
        reoType = GetOrientationTypeMonotonic(currWordsRange);
      } else {
        reoType = GetOrientationTypeLeftRight(currWordsRange);
      }
      CopyScores(scores, topt, reoType);
    }
  }

  return new PhraseBasedReorderingState(this, topt);
}

LexicalReorderingState::ReorderingType PhraseBasedReorderingState::GetOrientationTypeMSD(WordsRange currRange) const
{
  if (m_first) {
    if (currRange.GetStartPos() == 0) {
      return M;
    } else {
      return D;
    }
  }
  if (m_prevRange.GetEndPos() == currRange.GetStartPos()-1) {
    return M;
  } else if (m_prevRange.GetStartPos() == currRange.GetEndPos()+1) {
    return S;
  }
  return D;
}

LexicalReorderingState::ReorderingType PhraseBasedReorderingState::GetOrientationTypeMSLR(WordsRange currRange) const
{
  if (m_first) {
    if (currRange.GetStartPos() == 0) {
      return M;
    } else {
      return DR;
    }
  }
  if (m_prevRange.GetEndPos() == currRange.GetStartPos()-1) {
    return M;
  } else if (m_prevRange.GetStartPos() == currRange.GetEndPos()+1) {
    return S;
  } else if (m_prevRange.GetEndPos() < currRange.GetStartPos()) {
    return DR;
  }
  return DL;
}


LexicalReorderingState::ReorderingType PhraseBasedReorderingState::GetOrientationTypeMonotonic(WordsRange currRange) const
{
  if ((m_first && currRange.GetStartPos() == 0) ||
      (m_prevRange.GetEndPos() == currRange.GetStartPos()-1)) {
    return M;
  }
  return NM;
}

LexicalReorderingState::ReorderingType PhraseBasedReorderingState::GetOrientationTypeLeftRight(WordsRange currRange) const
{
  if (m_first ||
      (m_prevRange.GetEndPos() <= currRange.GetStartPos())) {
    return R;
  }
  return L;
}

///////////////////////////
//BidirectionalReorderingState

int BidirectionalReorderingState::Compare(const FFState& o) const
{
  if (&o == this)
    return 0;

  const BidirectionalReorderingState &other = dynamic_cast<const BidirectionalReorderingState &>(o);
  if(m_backward->Compare(*other.m_backward) < 0)
    return -1;
  else if(m_backward->Compare(*other.m_backward) > 0)
    return 1;
  else
    return m_forward->Compare(*other.m_forward);
}

LexicalReorderingState* BidirectionalReorderingState::Expand(const TranslationOption& topt, Scores& scores) const
{
  LexicalReorderingState *newbwd = m_backward->Expand(topt, scores);
  LexicalReorderingState *newfwd = m_forward->Expand(topt, scores);
  return new BidirectionalReorderingState(m_configuration, newbwd, newfwd, m_offset);
}

///////////////////////////
//HierarchicalReorderingBackwardState

HierarchicalReorderingBackwardState::HierarchicalReorderingBackwardState(const HierarchicalReorderingBackwardState *prev,
    const TranslationOption &topt, ReorderingStack reoStack)
  : LexicalReorderingState(prev, topt),  m_reoStack(reoStack) {}

HierarchicalReorderingBackwardState::HierarchicalReorderingBackwardState(const LexicalReorderingConfiguration &config, size_t offset)
  : LexicalReorderingState(config, LexicalReorderingConfiguration::Backward, offset) {}


int HierarchicalReorderingBackwardState::Compare(const FFState& o) const
{
  const HierarchicalReorderingBackwardState& other = dynamic_cast<const HierarchicalReorderingBackwardState&>(o);
  return m_reoStack.Compare(other.m_reoStack);
}

LexicalReorderingState* HierarchicalReorderingBackwardState::Expand(const TranslationOption& topt, Scores& scores) const
{

  HierarchicalReorderingBackwardState* nextState = new HierarchicalReorderingBackwardState(this, topt, m_reoStack);
  ReorderingType reoType;
  const LexicalReorderingConfiguration::ModelType modelType = m_configuration.GetModelType();

  int reoDistance = nextState->m_reoStack.ShiftReduce(topt.GetSourceWordsRange());

  if (modelType == LexicalReorderingConfiguration::MSD) {
    reoType = GetOrientationTypeMSD(reoDistance);
  } else if (modelType == LexicalReorderingConfiguration::MSLR) {
    reoType = GetOrientationTypeMSLR(reoDistance);
  } else if (modelType == LexicalReorderingConfiguration::LeftRight) {
    reoType = GetOrientationTypeLeftRight(reoDistance);
  } else {
    reoType = GetOrientationTypeMonotonic(reoDistance);
  }

  CopyScores(scores, topt, reoType);
  return nextState;
}

LexicalReorderingState::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeMSD(int reoDistance) const
{
  if (reoDistance == 1) {
    return M;
  } else if (reoDistance == -1) {
    return S;
  }
  return D;
}

LexicalReorderingState::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeMSLR(int reoDistance) const
{
  if (reoDistance == 1) {
    return M;
  } else if (reoDistance == -1) {
    return S;
  } else if (reoDistance > 1) {
    return DR;
  }
  return DL;
}

LexicalReorderingState::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeMonotonic(int reoDistance) const
{
  if (reoDistance == 1) {
    return M;
  }
  return NM;
}

LexicalReorderingState::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeLeftRight(int reoDistance) const
{
  if (reoDistance >= 1) {
    return R;
  }
  return L;
}




///////////////////////////
//HierarchicalReorderingForwardState

HierarchicalReorderingForwardState::HierarchicalReorderingForwardState(const LexicalReorderingConfiguration &config, size_t size, size_t offset)
  : LexicalReorderingState(config, LexicalReorderingConfiguration::Forward, offset), m_first(true), m_prevRange(NOT_FOUND,NOT_FOUND), m_coverage(size) {}

HierarchicalReorderingForwardState::HierarchicalReorderingForwardState(const HierarchicalReorderingForwardState *prev, const TranslationOption &topt)
  : LexicalReorderingState(prev, topt), m_first(false), m_prevRange(topt.GetSourceWordsRange()), m_coverage(prev->m_coverage)
{
  const WordsRange currWordsRange = topt.GetSourceWordsRange();
  m_coverage.SetValue(currWordsRange.GetStartPos(), currWordsRange.GetEndPos(), true);
}

int HierarchicalReorderingForwardState::Compare(const FFState& o) const
{
  if (&o == this)
    return 0;

  const HierarchicalReorderingForwardState* other = dynamic_cast<const HierarchicalReorderingForwardState*>(&o);
  UTIL_THROW_IF2(other == NULL, "Wrong state type");

  if (m_prevRange == other->m_prevRange) {
    return ComparePrevScores(other->m_prevScore);
  } else if (m_prevRange < other->m_prevRange) {
    return -1;
  }
  return 1;
}

// For compatibility with the phrase-based reordering model, scoring is one step delayed.
// The forward model takes determines orientations heuristically as follows:
//  mono:   if the next phrase comes after the conditioning phrase and
//          - there is a gap to the right of the conditioning phrase, or
//          - the next phrase immediately follows it
//  swap:   if the next phrase goes before the conditioning phrase and
//          - there is a gap to the left of the conditioning phrase, or
//          - the next phrase immediately precedes it
//  dright: if the next phrase follows the conditioning phrase and other stuff comes in between
//  dleft:  if the next phrase precedes the conditioning phrase and other stuff comes in between

LexicalReorderingState* HierarchicalReorderingForwardState::Expand(const TranslationOption& topt, Scores& scores) const
{
  const LexicalReorderingConfiguration::ModelType modelType = m_configuration.GetModelType();
  const WordsRange currWordsRange = topt.GetSourceWordsRange();
  // keep track of the current coverage ourselves so we don't need the hypothesis
  WordsBitmap coverage = m_coverage;
  coverage.SetValue(currWordsRange.GetStartPos(), currWordsRange.GetEndPos(), true);

  ReorderingType reoType;

  if (m_first) {
    ClearScores(scores);
  } else {
    if (modelType == LexicalReorderingConfiguration::MSD) {
      reoType = GetOrientationTypeMSD(currWordsRange, coverage);
    } else if (modelType == LexicalReorderingConfiguration::MSLR) {
      reoType = GetOrientationTypeMSLR(currWordsRange, coverage);
    } else if (modelType == LexicalReorderingConfiguration::Monotonic) {
      reoType = GetOrientationTypeMonotonic(currWordsRange, coverage);
    } else {
      reoType = GetOrientationTypeLeftRight(currWordsRange, coverage);
    }

    CopyScores(scores, topt, reoType);
  }

  return new HierarchicalReorderingForwardState(this, topt);
}

LexicalReorderingState::ReorderingType HierarchicalReorderingForwardState::GetOrientationTypeMSD(WordsRange currRange, WordsBitmap coverage) const
{
  if (currRange.GetStartPos() > m_prevRange.GetEndPos() &&
      (!coverage.GetValue(m_prevRange.GetEndPos()+1) || currRange.GetStartPos() == m_prevRange.GetEndPos()+1)) {
    return M;
  } else if (currRange.GetEndPos() < m_prevRange.GetStartPos() &&
             (!coverage.GetValue(m_prevRange.GetStartPos()-1) || currRange.GetEndPos() == m_prevRange.GetStartPos()-1)) {
    return S;
  }
  return D;
}

LexicalReorderingState::ReorderingType HierarchicalReorderingForwardState::GetOrientationTypeMSLR(WordsRange currRange, WordsBitmap coverage) const
{
  if (currRange.GetStartPos() > m_prevRange.GetEndPos() &&
      (!coverage.GetValue(m_prevRange.GetEndPos()+1) || currRange.GetStartPos() == m_prevRange.GetEndPos()+1)) {
    return M;
  } else if (currRange.GetEndPos() < m_prevRange.GetStartPos() &&
             (!coverage.GetValue(m_prevRange.GetStartPos()-1) || currRange.GetEndPos() == m_prevRange.GetStartPos()-1)) {
    return S;
  } else if (currRange.GetStartPos() > m_prevRange.GetEndPos()) {
    return DR;
  }
  return DL;
}

LexicalReorderingState::ReorderingType HierarchicalReorderingForwardState::GetOrientationTypeMonotonic(WordsRange currRange, WordsBitmap coverage) const
{
  if (currRange.GetStartPos() > m_prevRange.GetEndPos() &&
      (!coverage.GetValue(m_prevRange.GetEndPos()+1) || currRange.GetStartPos() == m_prevRange.GetEndPos()+1)) {
    return M;
  }
  return NM;
}

LexicalReorderingState::ReorderingType HierarchicalReorderingForwardState::GetOrientationTypeLeftRight(WordsRange currRange, WordsBitmap /* coverage */) const
{
  if (currRange.GetStartPos() > m_prevRange.GetEndPos()) {
    return R;
  }
  return L;
}


}
