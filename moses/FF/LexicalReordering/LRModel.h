#pragma once
#include <string>
#include <map>
#include <boost/scoped_ptr.hpp>

namespace Moses
{
class Range;
class Bitmap;
class InputType;
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

}

