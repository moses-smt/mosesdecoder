#include "RuleMap.h"
#include <string>

namespace Moses
{

RuleMap::~RuleMap()
{
    boost::unordered_map< std::string, std::vector<std::string> * >::iterator itr_clean;
    for(itr_clean = m_targetsForSource.begin(); itr_clean != m_targetsForSource.end(); itr_clean++)
    {
        itr_clean->second->clear();
        delete(itr_clean->second);
    }

}

void RuleMap::AddRule(std::string &sourceSide, std::string &targetRepresentation)
{
    //check if source is in map
    //if not create vector with targetPhrase
    boost::unordered_map< std::string,std::vector<std::string>* >::const_iterator gotSource = m_targetsForSource.find (sourceSide);

    //If source is not im map, insert new vector
    if ( gotSource == m_targetsForSource.end() )
    {
        //std::cout << "Source not found : insert new pair" << std::endl;
        std::vector<std::string> * targetAccumulator = new std::vector<std::string>;
        targetAccumulator->push_back(targetRepresentation);
        m_targetsForSource.insert(std::make_pair(sourceSide,targetAccumulator));
    }
    else
    {
        //std::cout << "Source found : accumulate target" << std::endl;
        gotSource->second->push_back(targetRepresentation);
    }

    //if yes
    //m_targetsForSource.insert();
}

}//end namespace
