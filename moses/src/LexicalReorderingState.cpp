
#include <vector>
#include <string>
#include <cassert>

#include "FFState.h"
#include "Hypothesis.h"
#include "WordsRange.h"
#include "ReorderingStack.h"

#include "LexicalReorderingState.h"

namespace Moses {

  LexicalReorderingState* LexicalReorderingState::CreateLexicalReorderingState(std::vector<std::string>& config, LexicalReordering::Direction dir) {
  
  ModelType mt = None;
  bool phraseBased = true;

  assert(dir != LexicalReordering::Bidirectional);

  for (int i=0; i<config.size(); ++i) {
    if (config[i] == "hier") {
      phraseBased == false;
    } else if (config[i] == "phrase") {
      phraseBased == true;
    } else if (config[i] == "msd") {
      mt = MSD;
    } else if (config[i] == "mslr") {
      mt = MSLR;
    } else if (config[i] == "monotonicity") {
      mt = Monotonic;
    } else if (config[i] == "leftright") {
      mt = LeftRight;
    }
    else {
      UserMessage::Add("Illegal part in the lexical reordering configuration string: "+config[i]);
      exit(1);      
    }
  }

  if (mt == None) { 
    UserMessage::Add("You need to specify the type of the reordering model (msd, monotonicity,...)");
    exit(1);
  }
  // create the correct model!!!

  if (phraseBased) {  //Same for forward and backward
    return new PhraseBasedReorderingState(mt);
  } else {
    if (dir == LexicalReordering::Backward) {
      return new HierarchicalReorderingBackwardState(mt);
    } else { //Forward
      return new HierarchicalReorderingForwardState(mt);
    }
  } 

}

PhraseBasedReorderingState::PhraseBasedReorderingState(ModelType mt, WordsRange wr)
  : LexicalReorderingState(mt), m_prevRange(wr), m_first(false) {};


PhraseBasedReorderingState::PhraseBasedReorderingState(ModelType mt)
  : LexicalReorderingState(mt), m_first(true), m_prevRange(WordsRange(NOT_FOUND,NOT_FOUND)) {};


int PhraseBasedReorderingState::Compare(const FFState& o) const {
  const PhraseBasedReorderingState& other = static_cast<const PhraseBasedReorderingState&>(o);
  if (m_prevRange == other.m_prevRange) {
    return 0;
  }
  else if (m_prevRange < other.m_prevRange) {
    return -1;
  }
  return 1;
}

LexicalReorderingState* PhraseBasedReorderingState::Expand(const Hypothesis& hypo, 
							   LexicalReordering::ReorderingType& reoType) const {

  const WordsRange currWordsRange = hypo.GetCurrSourceWordsRange();

  if (m_modelType == MSD) {
    reoType = GetOrientationTypeMSD(currWordsRange);
  }
  else if (m_modelType == MSLR) {
    reoType = GetOrientationTypeMSLR(currWordsRange);
  }
  else if (m_modelType == Monotonic) {
    reoType = GetOrientationTypeMonotonic(currWordsRange);
  }
  else  {
    reoType = GetOrientationTypeLeftRight(currWordsRange);
  }

  return new PhraseBasedReorderingState(m_modelType, currWordsRange);
}

LexicalReordering::ReorderingType PhraseBasedReorderingState::GetOrientationTypeMSD(WordsRange currRange) const {
  if (m_first) {
    if (currRange.GetStartPos() == 0) {
      return M;
    }
    else {
      return D;
    }
  }
  if (m_prevRange.GetEndPos() == currRange.GetStartPos()-1) {
    return M;
  }
  else if (m_prevRange.GetStartPos() == currRange.GetEndPos()+1) {
    return S;
  }
  return D;
}

LexicalReordering::ReorderingType PhraseBasedReorderingState::GetOrientationTypeMSLR(WordsRange currRange) const {
  if (m_first) {
    if (currRange.GetStartPos() == 0) {
      return M;
    }
    else {
      return DR;
    }
  }
  if (m_prevRange.GetEndPos() == currRange.GetStartPos()-1) {
    return M;
  }
  else if (m_prevRange.GetStartPos() == currRange.GetEndPos()+1) {
    return S;
  }
  else if (m_prevRange.GetEndPos() < currRange.GetStartPos()) {
    return DR;
  }
  return DL;
}


LexicalReordering::ReorderingType PhraseBasedReorderingState::GetOrientationTypeMonotonic(WordsRange currRange) const {
  if ((m_first && currRange.GetStartPos() == 0) ||
      (m_prevRange.GetEndPos() == currRange.GetStartPos()-1)) {
      return M;
  }
  return NM;
}

LexicalReordering::ReorderingType PhraseBasedReorderingState::GetOrientationTypeLeftRight(WordsRange currRange) const {
  if (m_first ||
      (m_prevRange.GetEndPos() <= currRange.GetStartPos())) {
      return R;
  }
  return L;
}


  ///////////////////////////
//HierarchicalReorderingBackwardState

HierarchicalReorderingBackwardState::HierarchicalReorderingBackwardState(ModelType mt, ReorderingStack reoStack)
  : LexicalReorderingState(mt),  m_reoStack(reoStack) {};

HierarchicalReorderingBackwardState::HierarchicalReorderingBackwardState(ModelType mt)
  : LexicalReorderingState(mt) {};


int HierarchicalReorderingBackwardState::Compare(const FFState& o) const {
  const HierarchicalReorderingBackwardState& other = dynamic_cast<const HierarchicalReorderingBackwardState&>(o);
  return m_reoStack.Compare(other.m_reoStack);
}

LexicalReorderingState* HierarchicalReorderingBackwardState::Expand(const Hypothesis& hypo, 
							    LexicalReordering::ReorderingType& reoType) const {

  //  const WordsRange currWordsRange  = hypo.GetCurrSourceWordsRange();
  HierarchicalReorderingBackwardState* nextState = new HierarchicalReorderingBackwardState(m_modelType, m_reoStack);

  int reoDistance = nextState->m_reoStack.ShiftReduce(hypo.GetCurrSourceWordsRange());

  if (m_modelType == MSD) {
    reoType = GetOrientationTypeMSD(reoDistance);
  }
  else if (m_modelType == MSLR) {
    reoType = GetOrientationTypeMSLR(reoDistance);
  }
  else if (m_modelType == LeftRight) {
    reoType = GetOrientationTypeLeftRight(reoDistance);  
  }
  else {
    reoType = GetOrientationTypeMonotonic(reoDistance);
  }

  return nextState;
}

LexicalReordering::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeMSD(int reoDistance) const {
  if (reoDistance == 1) {
    return M;
  }
  else if (reoDistance == -1) {
    return S;
  }
  return D;
}

LexicalReordering::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeMSLR(int reoDistance) const {
  if (reoDistance == 1) {
    return M;
  }
  else if (reoDistance == -1) {
    return S;
  }
  else if (reoDistance > 1) {
    return DR;
  }
  return DL;
}

LexicalReordering::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeMonotonic(int reoDistance) const {
  if (reoDistance == 1) {
    return M;
  }    
  return NM;
}

LexicalReordering::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeLeftRight(int reoDistance) const {
  if (reoDistance >= 1) {
    return R;
  }    
  return L;
}




  ///////////////////////////
//HierarchicalReorderingForwardState

HierarchicalReorderingForwardState::HierarchicalReorderingForwardState(ModelType mt)
  : LexicalReorderingState(mt) {};


int HierarchicalReorderingForwardState::Compare(const FFState& o) const {
  return 0;
}

LexicalReorderingState* HierarchicalReorderingForwardState::Expand(const Hypothesis& hypo, 
								   LexicalReordering::ReorderingType& reoType) const {
  //implement heuristic for forward probs!!
 
  const WordsRange currWordsRange = hypo.GetCurrSourceWordsRange();
  const WordsBitmap coverage = hypo.GetWordsBitmap();
  
  if (m_modelType == MSD) {
    reoType = GetOrientationTypeMSD(currWordsRange, coverage);
  }
  else if (m_modelType == MSLR) {
    reoType = GetOrientationTypeMSLR(currWordsRange, coverage);
  }
  else if (m_modelType == Monotonic) {
    reoType = GetOrientationTypeMonotonic(currWordsRange, coverage);
  }
  else  {
    reoType = GetOrientationTypeLeftRight(currWordsRange, coverage);
  }
  return new HierarchicalReorderingForwardState(m_modelType);
}


LexicalReordering::ReorderingType HierarchicalReorderingForwardState::GetOrientationTypeMSD(WordsRange currRange, WordsBitmap coverage) const {
  if (currRange.GetEndPos()+1 < coverage.GetSize() && coverage.GetValue(currRange.GetEndPos()+1)) {
    return M;
  } else if (currRange.GetEndPos()-1 >= 0 && coverage.GetValue(currRange.GetEndPos()-1)) {
    return S;
  }
  return D;
}

LexicalReordering::ReorderingType HierarchicalReorderingForwardState::GetOrientationTypeMSLR(WordsRange currRange, WordsBitmap coverage) const {
  if (currRange.GetEndPos()+1 < coverage.GetSize() && coverage.GetValue(currRange.GetEndPos()+1)) {
    return M;
  } else if (currRange.GetStartPos()-1 >= 0 && coverage.GetValue(currRange.GetStartPos()-1)) {
    return S;
  } else if (coverage.GetLastGapPos() > currRange.GetEndPos()) {
    return DR;
  }
  return DL;
}

LexicalReordering::ReorderingType HierarchicalReorderingForwardState::GetOrientationTypeMonotonic(WordsRange currRange, WordsBitmap coverage) const {
  if (currRange.GetEndPos()+1 < coverage.GetSize() && coverage.GetValue(currRange.GetEndPos()+1)) {
    return M;
  } 
  return NM;
}

LexicalReordering::ReorderingType HierarchicalReorderingForwardState::GetOrientationTypeLeftRight(WordsRange currRange, WordsBitmap coverage) const {
  if (currRange.GetEndPos()+1 < coverage.GetSize() && coverage.GetValue(currRange.GetEndPos()+1)) {
    return R;
  } else if (currRange.GetStartPos()-1 >= 0 && coverage.GetValue(currRange.GetStartPos()-1)) {
    return L;
  } else if (coverage.GetLastGapPos() > currRange.GetEndPos()) {
    return R;
  }
  return L;
}


}
