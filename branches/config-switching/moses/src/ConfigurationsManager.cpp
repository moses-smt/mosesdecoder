
#include <iostream> 
#include "ConfigurationsManager.h"


namespace Moses
{
bool ConfigurationsManager::GetPhraseDictionaries(const std::vector<std::string> &ttables, int id, int *cpId )
{
	m_configurations[id]->ttableFiles = ttables;

	if (id >0)
	{
		for (int i=0; i<id; i++)
		{
			if (equalVectors(ttables,m_configurations[i]->ttableFiles))
			{
				//*pDEntries = (m_configurations[i]->m_pDs);
				*cpId = i;
				std::cout<<"------got loaded phraseDics."<<std::endl;
				return true;
			}
		}
	}	
	std::cout<<"------these phraseDics not loaded."<<std::endl;
	return false;
}
bool ConfigurationsManager::GetReorderingModels(const std::vector<std::string> &distortionFiles, int id, int *cpId )
{	
	m_configurations[id]->distortionFiles = distortionFiles;

	if (id >0)
	{
		for (int i=0; i<id; i++)
		{
			if (equalVectors(distortionFiles,m_configurations[i]->distortionFiles))
			{
				//*pDEntries = (m_configurations[i]->m_pDs);
				*cpId = i;
				std::cout<<"------got loaded reorderings."<<std::endl;
				return true;
			}
		}
	}	
	std::cout<<"------these reordering models not loaded."<<std::endl;
	return false;
}

bool ConfigurationsManager::equalVectors(const std::vector<std::string> &newV, const std::vector<std::string> &oldV)
{
	if (newV.size() != oldV.size())
	  return false;

	bool found;
	for (size_t i=0; i<newV.size(); i++)
	{
		found = false;
		for (size_t j=0; j<oldV.size(); j++)
		{
		  if (newV[i].compare(oldV[i]) == 0)
		  {
		    found=true;
		    break;
		  }
		}
		if (!found) return false;
	}

	return true;
}

}

