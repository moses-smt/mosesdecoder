#pragma once

#include <string>
#include <cstdlib>
#include <vector>
#include <map>

#include <boost/thread/tss.hpp>

#include "moses/FF/FeatureFunction.h"

namespace Moses
{

template <class Value>
struct DefaultFactory {
  Value* operator()() {
    return new Value();
  }
};

template<class Value, class Factory = DefaultFactory<Value> >
class ThreadLocalByFeatureStorage
{
  public:

    typedef std::map<std::string, Value*> NameValueMap;
    typedef boost::thread_specific_ptr<NameValueMap> TSNameValueMap;
    
    ThreadLocalByFeatureStorage(FeatureFunction* ff,
                                Factory factory = Factory())
    : m_ff(ff), m_factory(factory) {}
    
    ~ThreadLocalByFeatureStorage() {
      if(m_nameMap.get()) {
        for(typename NameValueMap::iterator it = m_nameMap->begin();
            it != m_nameMap->end(); it++)
          delete it->second;
      }
    }
    
    virtual Value* GetStored() {
      if(!m_nameMap.get())
        m_nameMap.reset(new NameValueMap());
        
      typename NameValueMap::iterator it
        = m_nameMap->find(m_ff->GetScoreProducerDescription());
      
      if(it == m_nameMap->end()) {
          std::pair<typename NameValueMap::iterator, bool> ret;
          ret = m_nameMap->insert(
            std::make_pair(m_ff->GetScoreProducerDescription(), m_factory()));
          
          return ret.first->second;
      }
      else {
        return it->second;
      }
    }
    
    virtual const Value* GetStored() const {
      UTIL_THROW_IF2(!m_nameMap.get(),
                     "No thread local storage has been created for: "
                     << m_ff->GetScoreProducerDescription());
      
      typename NameValueMap::const_iterator it
        = m_nameMap->find(m_ff->GetScoreProducerDescription());
      
      UTIL_THROW_IF2(it == m_nameMap->end(),
                     "No features stored for: "
                     << m_ff->GetScoreProducerDescription());
      
      return it->second;
    }
    
  private:
    FeatureFunction* m_ff;
    Factory m_factory;
    static TSNameValueMap m_nameMap;
};

template <class Value, class Factory>
typename ThreadLocalByFeatureStorage<Value, Factory>::TSNameValueMap
ThreadLocalByFeatureStorage<Value, Factory>::m_nameMap;

}
