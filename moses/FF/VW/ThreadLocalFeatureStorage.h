#pragma once

#include <string>
#include <cstdlib>
#include <vector>
#include <map>

#include <boost/thread/tss.hpp>

#include "moses/FF/FeatureFunction.h"

namespace Moses
{
  
typedef std::vector<std::string> Features;
typedef std::map<std::string, Features> NameFeatureMap;
typedef boost::thread_specific_ptr<NameFeatureMap> TSNameFeatureMap;
  
class ThreadLocalFeatureStorage
{
  public:
    
    ThreadLocalFeatureStorage(FeatureFunction* ff) : m_ff(ff) {}
    
    virtual Features& GetStoredFeatures() {
      return (*m_nameMap)[m_ff->GetScoreProducerDescription()];
    }
    
    virtual const Features& GetStoredFeatures() const {
      NameFeatureMap::const_iterator it
        = m_nameMap->find(m_ff->GetScoreProducerDescription());
      
      UTIL_THROW_IF2(it == m_nameMap->end(),
                   "No features stored for: " << m_ff->GetScoreProducerDescription());
      
      return it->second;
    }
    
    virtual void InitializeForInput(InputType const& source) {
      if(!m_nameMap.get())
        m_nameMap.reset(new NameFeatureMap());
      
      (*m_nameMap)[m_ff->GetScoreProducerDescription()].clear();
    }
    
  private:
    FeatureFunction* m_ff;
    static TSNameFeatureMap m_nameMap;
};

}
