/*
 * LRModel.cpp
 *
 *  Created on: 23 Mar 2016
 *      Author: hieu
 */

#include "LRModel.h"
#include "../../legacy/Util2.h"

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

} /* namespace Moses2 */
