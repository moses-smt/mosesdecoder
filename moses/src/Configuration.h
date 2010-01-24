#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include <iostream>
#include <vector>
#include "TypeDef.h"
#include "Parameter.h"
#include "ScoreIndexManager.h"
#include "DecodeGraph.h"
#include "PhraseDictionary.h"
#include "LexicalReordering.h"

namespace Moses
{
class Configuration
{
	  public:
		// phraseTables, weights, parameter strings, cache pointers,
		// decodeStepVL, scoreIndexManager, searchAlgorithm
		// self_id
		std::vector<std::string> ttableFiles; // original parameter string
		std::vector<std::string> distortionFiles; // original parameter string
		std::vector<std::string> mappingVector;

		std::vector<PhraseDictionaryFeature*> m_pDs;
		std::vector<LexicalReordering*> m_reorders;
		std::vector<float> m_weights;
		SearchAlgorithm m_searchAlgorithm;
		std::vector<FactorType>	m_inputFactorOrder, m_outputFactorOrder;
		bool m_useTransOptCache; 
		size_t m_transOptCacheMaxSize; 
};
}
#endif
