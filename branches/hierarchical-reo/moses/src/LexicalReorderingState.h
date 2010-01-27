
#pragma once

#include <vector>
#include <string>

#include "FFState.h"
#include "Hypothesis.h"
#include "WordsRange.h"
#include "ReorderingStack.h"


namespace Moses
{
  //! Abstract class for lexical reordering model states
class LexicalReorderingState : public FFState {
  public:

    enum modelType {Monotonic, MSD, MSLR, LeftRight, None};

  LexicalReorderingState(modelType mt) 
    : m_modelType(mt) {}

    virtual int Compare(const FFState& o) const = 0;
    virtual LexicalReorderingState* expand(const Hypothesis& hypo, 
					   ReorderingType& reoType);

    LexicalReorderingState* CreateLexicalReorderingState(vector<string>& conf) const;
  protected:
    modelType m_modelType;

    //constants for the different type of reorderings (corresponding to indexes in the table file)
    static const ReorderingType M = 0;  // monotonic
    static const ReorderingType NM = 1; // non-monotonic
    static const ReorderingType S = 1;  // swap
    static const ReorderingType D = 2;  // discontinuous
    static const ReorderingType DR = 2; // discontinuous, left
    static const ReorderingType DL = 3; // discontinuous, right
    static const ReorderingType L = 0;  // left
    static const ReorderingType R = 1;  // right
};

//! State for the standard Moses implementation of lexical reordering models
//! (see Koehn et al, Edinburgh System Description for the 2005 NIST MT Evaluation)
class PhraseBasedReorderingState : public LexicalReorderingState {
  private:
    WordsRange m_prevRange;
  public:
    PhraseBasedReorderingState(modelType mt, WordsRange wr);
    virtual int Compare(const FFState& o) const;
    virtual LexicalReorderingState* expand(const Hypothesis& hypo, 
					   ReorderingType& reoType);

    ReorderingType GetOrientationTypeMSD(bool first, WordsRange currRange) const;
    ReorderingType GetOrientationTypeMSLR(bool first, WordsRange currRange) const;
    ReorderingType GetOrientationTypeMonotonic(bool first, WordsRange currRange) const;
    ReorderingType GetOrientationTypeLeftRight(bool first, WordsRange currRange) const;
};

  //! State for a hierarchical reordering model 
  //! (see Galley and Manning, A Simple and Effective Hierarchical Phrase Reordering Model, EMNLP 2008)
class HierarchicalReorderingState : public LexicalReorderingState {
  private:
    ReorderingStack m_reoStack;
  public:
    HierarchicalReorderingState(modelType mt, ReorderingStack reoStack);
    virtual int Compare(const FFState& o) const;
    virtual LexicalReorderingState* expand(const Hypothesis& hypo, 
					   ReorderingType& reoType);

  private:
    ReorderingType GetOrientationTypeMSD(int reoDistance) const;
    ReorderingType GetOrientationTypeMSLR(int reoDistance) const;
    ReorderingType GetOrientationTypeMonotonic(int reoDistance) const;
    ReorderingType GetOrientationTypeLeftRight(int reoDistance) const;
};


}
