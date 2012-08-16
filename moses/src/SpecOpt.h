#ifndef moses_SpecOpt_h
#define moses_SpecOpt_h

#include <vector>
#include <map>
#include <string>

#ifdef WITH_THREADS
#include <boost/thread/mutex.hpp>
#endif

#include "InputType.h"
#include "Parameter.h"
#include "TranslationSystem.h"

namespace Moses
{
  
struct WeightInfo {
  std::string name;
  size_t      ffIndex;
  size_t      ffWeightIndex;
  float       value;
  
  std::string GetDescription() {
    std::stringstream out;
    out << name << ":" << ffIndex << ":" << ffWeightIndex;
    return out.str();
  }
};

typedef std::vector<WeightInfo> WeightInfos;
    
class SpecOpt {
private:
  #ifdef WITH_THREADS
  #ifdef BOOST_HAS_PTHREADS
  static boost::mutex s_namedMutex;
  #endif
  #endif
  
  static std::map<std::string, SpecOpt*> s_named;
  
  bool m_hasSpecificOptions;
  std::string m_name;
  bool m_changed;
  
  std::string m_translationSystemId;
  WeightInfos m_weights;
  Parameter m_switches;
  
  
  
  void ProcessAndStripSpecificOptions(std::string &line);
  WeightInfos parseWeights(std::string s);
  
public:
  SpecOpt()
  : m_hasSpecificOptions(false), m_changed(false),
  m_translationSystemId(TranslationSystem::DEFAULT) { }
    
  SpecOpt(std::string& line)
  : m_hasSpecificOptions(false), m_changed(false),
  m_translationSystemId(TranslationSystem::DEFAULT)
  {
    ProcessAndStripSpecificOptions(line);
  }

  static void InitializeFromFile(std::string filename);

  bool HasSpecificOptions() const
  {
    return m_hasSpecificOptions;
  }
  
  const WeightInfos& GetWeights() const
  {
    #ifdef WITH_THREADS
    #ifdef BOOST_HAS_PTHREADS
    boost::mutex::scoped_lock lock(s_namedMutex);
    #endif
    #endif

    if(m_name.size() && s_named.count(m_name))
      return s_named[m_name]->m_weights;
    return m_weights;
  }
  
  const Parameter& GetSwitches() const
  {
    #ifdef WITH_THREADS
    #ifdef BOOST_HAS_PTHREADS
    boost::mutex::scoped_lock lock(s_namedMutex);
    #endif
    #endif

    if(m_name.size() && s_named.count(m_name))
      return s_named[m_name]->m_switches;
    return m_switches;
  }
  
  std::string GetTranslationSystemId() const
  {
    #ifdef WITH_THREADS
    #ifdef BOOST_HAS_PTHREADS
    boost::mutex::scoped_lock lock(s_namedMutex);
    #endif
    #endif

    if(m_name.size() && s_named.count(m_name))
      return s_named[m_name]->m_translationSystemId;
    return m_translationSystemId;
  }
};

}
#endif
