
#ifndef _CONFIGURATIONS_MANAGER_H_
#define _CONFIGURATIONS_MANAGER_H_

#include "Configuration.h"
#include "Parameter.h"
#include "ScoreIndexManager.h"
#include "DecodeGraph.h"
#include "PhraseDictionaryMemory.h"
#include "LexicalReordering.h"

namespace Moses
{

class ConfigurationsManager
{
  public:
	ConfigurationsManager(){}

	const std::vector<float>& GetAllWeights(int id) const
	{
		return m_configurations[id]->m_weights;
	}
	SearchAlgorithm GetSearchAlgorithm(int id) const {
		return m_configurations[id]->m_searchAlgorithm;
	}
	bool GetUseTransOptCache(int id) const {
		return m_configurations[id]->m_useTransOptCache;
	}
	bool GetPhraseDictionaries(const std::vector<std::string> &ttables, int id, int *cpId );
	bool GetReorderingModels(const std::vector<std::string> &distortionFiles, int id, int *cpId );
	bool IsValidId(int id) const {
		return (id>=0) && (id<(int)m_configurations.size());
	}
	std::vector<Configuration*> m_configurations;

  private:
	bool equalVectors(const std::vector<std::string> &newV, const std::vector<std::string> &oldV);
};

}

#endif

