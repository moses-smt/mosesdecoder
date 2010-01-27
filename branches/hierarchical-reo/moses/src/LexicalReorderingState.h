
#pragma once

#include <vector>
#include <string>

#include "FFState.h"
#include "Hypothesis.h"
#include "WordsRange.h"
#include "WordsBitmap.h"
#include "ReorderingStack.h"


namespace Moses
{
  //! Abstract class for lexical reordering model states
class LexicalReorderingState : public FFState {
  public:

    enum ModelType {Monotonic, MSD, MSLR, LeftRight, None};

  inline LexicalReorderingState(ModelType mt) 
    : m_modelType(mt) {}

    virtual int Compare(const FFState& o) const = 0;
    virtual LexicalReorderingState* Expand(const Hypothesis& hypo, 
					   LexicalReordering::ReorderingType& reoType) const;

    static LexicalReorderingState* CreateLexicalReorderingState(std::vector<std::string>& config,
								LexicalReordering::Direction dir);
  protected:
    ModelType m_modelType;

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
    bool m_first;
  public:
    PhraseBasedReorderingState(ModelType mt);
    PhraseBasedReorderingState(ModelType mt, WordsRange wr);
    virtual int Compare(const FFState& o) const;
    virtual LexicalReorderingState* Expand(const Hypothesis& hypo, 
					   LexicalReordering::ReorderingType& reoType) const;

    LexicalReordering::ReorderingType GetOrientationTypeMSD(WordsRange currRange) const;
    LexicalReordering::ReorderingType GetOrientationTypeMSLR(WordsRange currRange) const;
    LexicalReordering::ReorderingType GetOrientationTypeMonotonic(WordsRange currRange) const;
    LexicalReordering::ReorderingType GetOrientationTypeLeftRight(WordsRange currRange) const;
};

  //! State for a hierarchical reordering model 
  //! (see Galley and Manning, A Simple and Effective Hierarchical Phrase Reordering Model, EMNLP 2008)
  //!backward state (conditioned on the previous phrase)
class HierarchicalReorderingBackwardState : public LexicalReorderingState {
  private:
    ReorderingStack m_reoStack;
  public:
    HierarchicalReorderingBackwardState(ModelType mt);
    HierarchicalReorderingBackwardState(ModelType mt, ReorderingStack reoStack);
    virtual int Compare(const FFState& o) const;
    virtual LexicalReorderingState* Expand(const Hypothesis& hypo, 
					   LexicalReordering::ReorderingType& reoType) const;

  private:
    LexicalReordering::ReorderingType GetOrientationTypeMSD(int reoDistance) const;
    LexicalReordering::ReorderingType GetOrientationTypeMSLR(int reoDistance) const;
    LexicalReordering::ReorderingType GetOrientationTypeMonotonic(int reoDistance) const;
    LexicalReordering::ReorderingType GetOrientationTypeLeftRight(int reoDistance) const;
};


  //!backward state (conditioned on the next phrase)
class HierarchicalReorderingForwardState : public LexicalReorderingState {
    
  public:
    HierarchicalReorderingForwardState(ModelType mt);
    virtual int Compare(const FFState& o) const;
    virtual LexicalReorderingState* Expand(const Hypothesis& hypo, 
					   LexicalReordering::ReorderingType& reoType) const;

  private:
    LexicalReordering::ReorderingType GetOrientationTypeMSD(WordsRange currRange, WordsBitmap coverage) const;
    LexicalReordering::ReorderingType GetOrientationTypeMSLR(WordsRange currRange, WordsBitmap coverage) const;
    LexicalReordering::ReorderingType GetOrientationTypeMonotonic(WordsRange currRange, WordsBitmap coverage) const;
    LexicalReordering::ReorderingType GetOrientationTypeLeftRight(WordsRange currRange, WordsBitmap coverage) const;
};

}
