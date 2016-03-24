/*
 * LRModel.cpp
 *
 *  Created on: 23 Mar 2016
 *      Author: hieu
 */

#include "LRModel.h"
#include "../../legacy/Util2.h"
#include "../../legacy/Range.h"
#include "../../InputType.h"
#include "util/exception.hh"

namespace Moses2 {

LRModel::LRModel(const std::string &modelType)
: m_modelType(None)
, m_phraseBased(true)
, m_collapseScores(false)
, m_direction(Backward)
{
  std::vector<std::string> config = Tokenize(modelType, "-");

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

LRModel::~LRModel() {
	// TODO Auto-generated destructor stub
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

LRState *
LRModel::
CreateLRState(const InputType &input) const
{
  LRState *bwd = NULL, *fwd = NULL;
  size_t offset = 0;

}

} /* namespace Moses2 */
