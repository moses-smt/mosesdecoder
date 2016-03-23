/*
 * LRModel.h
 *
 *  Created on: 23 Mar 2016
 *      Author: hieu
 */
#pragma once
#include <string>

namespace Moses2 {

class LRModel {
public:
	  enum ModelType { Monotonic, MSD, MSLR, LeftRight, None };
	  enum Direction { Forward, Backward, Bidirectional };
	  enum Condition { F, E, FE };

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

protected:

  bool m_phraseBased;
  bool m_collapseScores;
  ModelType m_modelType;
  Direction m_direction;
  Condition m_condition;

};

} /* namespace Moses2 */

