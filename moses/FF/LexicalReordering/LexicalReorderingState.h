// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once
#include <vector>
#include <string>

#include <boost/scoped_ptr.hpp>

#include "moses/Hypothesis.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Range.h"
#include "moses/Bitmap.h"
#include "moses/TranslationOption.h"
#include "moses/FF/FFState.h"
#include "ReorderingStack.h"

namespace Moses
{
class LRState;
class LexicalReordering;
class SparseReordering;

//! Factory class for lexical reordering states
class LRModel
{
public:
  friend class LexicalReordering;
  enum ModelType { Monotonic, MSD, MSLR, LeftRight, None };
  enum Direction { Forward, Backward, Bidirectional };
  enum Condition { F, E, FE };

  // constants for the different types of reordering
  // (correspond to indices in the respective table)
#if 0
  typedef int ReorderingType;
  static const ReorderingType M   = 0; // monotonic
  static const ReorderingType NM  = 1; // non-monotonic
  static const ReorderingType S   = 1; // swap
  static const ReorderingType D   = 2; // discontinuous
  static const ReorderingType DL  = 2; // discontinuous, left
  static const ReorderingType DR  = 3; // discontinuous, right
  static const ReorderingType R   = 0; // right
  static const ReorderingType L   = 1; // left
  static const ReorderingType MAX = 3; // largest possible
#else
  enum ReorderingType {
    M    = 0, // monotonic
    NM   = 1, // non-monotonic
    S    = 1, // swap
    D    = 2, // discontinuous
    DL   = 2, // discontinuous, left
    DR   = 3, // discontinuous, right
    R    = 0, // right
    L    = 1, // left
    MAX  = 3, // largest possible
    NONE = 4  // largest possible
  };
#endif
  // determine orientation, depending on model:


  ReorderingType // for first phrase in phrase-based
  GetOrientation(Range const& cur) const;

  ReorderingType // for non-first phrases in phrase-based
  GetOrientation(Range const& prev, Range const& cur) const;

  ReorderingType // for HReorderingForwardState
  GetOrientation(Range const& prev, Range const& cur,
                 Bitmap const& cov) const;

  ReorderingType // for HReorderingBackwarddState
  GetOrientation(int const reoDistance) const;

  LRModel(const std::string &modelType);

  void
  ConfigureSparse(const std::map<std::string,std::string>& sparseArgs,
                  const LexicalReordering* producer);

  LRState*
  CreateLRState(const InputType &input) const;

  size_t GetNumberOfTypes() const;
  size_t GetNumScoreComponents() const;
  void SetAdditionalScoreComponents(size_t number);

  LexicalReordering*
  GetScoreProducer() const {
    return m_scoreProducer;
  }

  ModelType GetModelType() const {
    return m_modelType;
  }
  Direction GetDirection() const {
    return m_direction;
  }
  Condition GetCondition() const {
    return m_condition;
  }

  bool
  IsPhraseBased()  const {
    return m_phraseBased;
  }

  bool
  CollapseScores() const {
    return m_collapseScores;
  }

  SparseReordering const*
  GetSparseReordering() const {
    return m_sparse.get();
  }

private:
  void
  SetScoreProducer(LexicalReordering* scoreProducer) {
    m_scoreProducer = scoreProducer;
  }

  std::string const&
  GetModelString() const {
    return m_modelString;
  }

  std::string m_modelString;
  LexicalReordering *m_scoreProducer;
  ModelType m_modelType;
  bool m_phraseBased;
  bool m_collapseScores;
  Direction m_direction;
  Condition m_condition;
  size_t m_additionalScoreComponents;
  boost::scoped_ptr<SparseReordering> m_sparse;
};

//! Abstract class for lexical reordering model states
class LRState : public FFState
{
public:

  typedef LRModel::ReorderingType ReorderingType;

  virtual
  LRState*
  Expand(const TranslationOption& hypo, const InputType& input,
         ScoreComponentCollection* scores) const = 0;

  static
  LRState*
  CreateLRState(const std::vector<std::string>& config,
                LRModel::Direction dir,
                const InputType &input);

protected:

  const LRModel& m_configuration;

  // The following is the true direction of the object, which can be
  // Backward or Forward even if the Configuration has Bidirectional.
  LRModel::Direction m_direction;
  size_t m_offset;
  //forward scores are conditioned on prev option, so need to remember it
  const TranslationOption *m_prevOption;

  inline
  LRState(const LRState *prev,
          const TranslationOption &topt)
    : m_configuration(prev->m_configuration)
    , m_direction(prev->m_direction)
    , m_offset(prev->m_offset)
    , m_prevOption(&topt)
  { }

  inline
  LRState(const LRModel &config,
          LRModel::Direction dir,
          size_t offset)
    : m_configuration(config)
    , m_direction(dir)
    , m_offset(offset)
    , m_prevOption(NULL)
  { }

  // copy the right scores in the right places, taking into account
  // forward/backward, offset, collapse
  void
  CopyScores(ScoreComponentCollection* scores,
             const TranslationOption& topt,
             const InputType& input, ReorderingType reoType) const;

  int
  ComparePrevScores(const TranslationOption *other) const;
};

//! @todo what is this?
class BidirectionalReorderingState
  : public LRState
{
private:
  const LRState *m_backward;
  const LRState *m_forward;
public:
  BidirectionalReorderingState(const LRModel &config,
                               const LRState *bw,
                               const LRState *fw, size_t offset)
    : LRState(config,
              LRModel::Bidirectional,
              offset)
    , m_backward(bw)
    , m_forward(fw)
  { }

  ~BidirectionalReorderingState() {
    delete m_backward;
    delete m_forward;
  }

  virtual size_t hash() const;
  virtual bool operator==(const FFState& other) const;

  LRState*
  Expand(const TranslationOption& topt, const InputType& input,
         ScoreComponentCollection*  scores) const;
};

//! State for the standard Moses implementation of lexical reordering models
//! (see Koehn et al, Edinburgh System Description for the 2005 NIST MT
//! Evaluation)
class PhraseBasedReorderingState
  : public LRState
{
private:
  Range m_prevRange;
  bool m_first;
public:
  static bool m_useFirstBackwardScore;
  PhraseBasedReorderingState(const LRModel &config,
                             LRModel::Direction dir,
                             size_t offset);
  PhraseBasedReorderingState(const PhraseBasedReorderingState *prev,
                             const TranslationOption &topt);

  virtual size_t hash() const;
  virtual bool operator==(const FFState& other) const;

  virtual
  LRState*
  Expand(const TranslationOption& topt,const InputType& input,
         ScoreComponentCollection*  scores) const;

  ReorderingType GetOrientationTypeMSD(Range currRange) const;
  ReorderingType GetOrientationTypeMSLR(Range currRange) const;
  ReorderingType GetOrientationTypeMonotonic(Range currRange) const;
  ReorderingType GetOrientationTypeLeftRight(Range currRange) const;
};

//! State for a hierarchical reordering model (see Galley and Manning, A
//! Simple and Effective Hierarchical Phrase Reordering Model, EMNLP 2008)
//! backward state (conditioned on the previous phrase)
class HReorderingBackwardState : public LRState
{
private:
  ReorderingStack m_reoStack;
public:
  HReorderingBackwardState(const LRModel &config, size_t offset);
  HReorderingBackwardState(const HReorderingBackwardState *prev,
                           const TranslationOption &topt,
                           ReorderingStack reoStack);
  virtual size_t hash() const;
  virtual bool operator==(const FFState& other) const;

  virtual LRState* Expand(const TranslationOption& hypo, const InputType& input,
                          ScoreComponentCollection*  scores) const;

private:
  ReorderingType GetOrientationTypeMSD(int reoDistance) const;
  ReorderingType GetOrientationTypeMSLR(int reoDistance) const;
  ReorderingType GetOrientationTypeMonotonic(int reoDistance) const;
  ReorderingType GetOrientationTypeLeftRight(int reoDistance) const;
};


//!forward state (conditioned on the next phrase)
class HReorderingForwardState : public LRState
{
private:
  bool m_first;
  Range m_prevRange;
  Bitmap m_coverage;

public:
  HReorderingForwardState(const LRModel &config, size_t sentenceLength,
                          size_t offset);
  HReorderingForwardState(const HReorderingForwardState *prev,
                          const TranslationOption &topt);

  virtual size_t hash() const;
  virtual bool operator==(const FFState& other) const;

  virtual LRState* Expand(const TranslationOption& hypo,
                          const InputType& input,
                          ScoreComponentCollection* scores) const;
};
}

