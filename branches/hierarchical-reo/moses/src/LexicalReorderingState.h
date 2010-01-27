
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
					   LexicalReordering::ReorderingType& reoType);

    LexicalReorderingState* CreateLexicalReorderingState(std::vector<std::string>& conf) const;
  protected:
    modelType m_modelType;

    //constants for the different type of reorderings (corresponding to indexes in the table file)
    static const LexicalReordering::ReorderingType M = 0;  // monotonic
    static const LexicalReordering::ReorderingType NM = 1; // non-monotonic
    static const LexicalReordering::ReorderingType S = 1;  // swap
    static const LexicalReordering::ReorderingType D = 2;  // discontinuous
    static const LexicalReordering::ReorderingType DR = 2; // discontinuous, left
    static const LexicalReordering::ReorderingType DL = 3; // discontinuous, right
    static const LexicalReordering::ReorderingType L = 0;  // left
    static const LexicalReordering::ReorderingType R = 1;  // right
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
					   LexicalReordering::ReorderingType& reoType);

    LexicalReordering::ReorderingType GetOrientationTypeMSD(bool first, WordsRange currRange) const;
    LexicalReordering::ReorderingType GetOrientationTypeMSLR(bool first, WordsRange currRange) const;
    LexicalReordering::ReorderingType GetOrientationTypeMonotonic(bool first, WordsRange currRange) const;
    LexicalReordering::ReorderingType GetOrientationTypeLeftRight(bool first, WordsRange currRange) const;
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
					   LexicalReordering::ReorderingType& reoType);

  private:
    LexicalReordering::ReorderingType GetOrientationTypeMSD(int reoDistance) const;
    LexicalReordering::ReorderingType GetOrientationTypeMSLR(int reoDistance) const;
    LexicalReordering::ReorderingType GetOrientationTypeMonotonic(int reoDistance) const;
    LexicalReordering::ReorderingType GetOrientationTypeLeftRight(int reoDistance) const;
};


}
