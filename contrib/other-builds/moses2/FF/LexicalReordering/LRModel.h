/*
 * LRModel.h
 *
 *  Created on: 23 Mar 2016
 *      Author: hieu
 */
#pragma once
#include <string>

namespace Moses2 {

class Sentence;
class Range;
class LRState;

class LRModel {
public:
  enum ModelType { Monotonic, MSD, MSLR, LeftRight, None };
  enum Direction { Forward, Backward, Bidirectional };
  enum Condition { F, E, FE };

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

	LRModel(const std::string &modelType);
	virtual ~LRModel();

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

	  size_t GetNumberOfTypes() const;

	  LRState*
	  CreateLRState(const Sentence &input) const;

protected:

  bool m_phraseBased;
  bool m_collapseScores;
  ModelType m_modelType;
  Direction m_direction;
  Condition m_condition;

  ReorderingType // for first phrase in phrase-based
  GetOrientation(Range const& cur) const;

  ReorderingType // for non-first phrases in phrase-based
  GetOrientation(Range const& prev, Range const& cur) const;

};

} /* namespace Moses2 */

