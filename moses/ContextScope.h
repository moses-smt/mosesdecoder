// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
// A class to store "local" information (such as task-specific caches).
// The idea is for each translation task to have a scope, which stores
// shared pointers to task-specific objects such as caches and priors.
// Since these objects are referenced via shared pointers, sopes can
// share information.
#pragma once

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/foreach.hpp>
#endif

// for some reason, the xmlrpc_c headers must be included AFTER the
// boost thread-related ones ...
#include "xmlrpc-c.h"

#include <map>
#include <boost/shared_ptr.hpp>
#include "TypeDef.h"
#include "Util.h"

#include "moses/StaticData.h"

namespace Moses
{
//the type weightmap_t  must be equal to the type topic_map_t of IRSTLM
typedef std::map<std::string,float> weightmap_t;
typedef std::map<std::string, weightmap_t*> weightmap_map_t;
//typedef std::map<std::string, SPTR<weightmap_t const> > weightmap_map_t;

class ContextScope
{
protected:
  typedef std::map<void const*, boost::shared_ptr<void> > scratchpad_t;
  typedef scratchpad_t::iterator iter_t;
  typedef scratchpad_t::value_type entry_t;
  typedef scratchpad_t::const_iterator const_iter_t;
  scratchpad_t m_scratchpad;
#ifdef WITH_THREADS
  mutable boost::shared_mutex m_lock;
#endif
    SPTR< std::map<std::string,float> const> m_context_weights;
    weightmap_t* m_normalized_context_weights;
//    weightmap_t* m_context_weights;
    weightmap_map_t* m_lm_context_weights;
    weightmap_map_t* m_normalized_lm_context_weights;
//  SPTR<weightmap_t const> m_context_weights;
//  SPTR<weightmap_map_t> m_lm_context_weights;
public:
  typedef boost::shared_ptr<ContextScope> ptr;
  template<typename T>
  boost::shared_ptr<void> const&
  set(void const* const key, boost::shared_ptr<T> const& val) {
#ifdef WITH_THREADS
    boost::unique_lock<boost::shared_mutex> lock(m_lock);
#endif
    return (m_scratchpad[key] = val);
  }

  template<typename T>
  boost::shared_ptr<T> const
  get(void const* key, bool CreateNewIfNecessary=false) {
#ifdef WITH_THREADS
    using boost::shared_mutex;
    using boost::upgrade_lock;
    // T const* key = reinterpret_cast<T const*>(xkey);
    upgrade_lock<shared_mutex> lock(m_lock);
#endif
    iter_t m = m_scratchpad.find(key);
    boost::shared_ptr< T > ret;
    if (m != m_scratchpad.end()) {
      if (m->second == NULL && CreateNewIfNecessary) {
#ifdef WITH_THREADS
        boost::upgrade_to_unique_lock<shared_mutex> xlock(lock);
#endif
        m->second.reset(new T);
      }
      ret = boost::static_pointer_cast< T >(m->second);
      return ret;
    }
    if (!CreateNewIfNecessary) return ret;
#ifdef WITH_THREADS
    boost::upgrade_to_unique_lock<shared_mutex> xlock(lock);
#endif
    ret.reset(new T);
    m_scratchpad[key] = ret;
    return ret;
  }

  ContextScope() {
//    m_context_weights = NULL; 
    m_normalized_context_weights = NULL; 
    m_lm_context_weights = NULL; 
    m_normalized_lm_context_weights = NULL; 
  }

  ContextScope(ContextScope const& other) {
#ifdef WITH_THREADS
    boost::unique_lock<boost::shared_mutex> lock1(this->m_lock);
    boost::unique_lock<boost::shared_mutex> lock2(other.m_lock);
#endif
    m_scratchpad = other.m_scratchpad;
  }

/*
  weightmap_t*
  GetContextWeights() {
    return m_context_weights;
  }
*/
  SPTR<std::map<std::string,float> const>
  GetContextWeights() {
    return m_context_weights;
  }

  weightmap_t*
  GetNormalizedContextWeights() {
    return m_normalized_context_weights;
  }

  weightmap_map_t*
  GetLMContextWeights() {
    return m_lm_context_weights;
  }

  weightmap_map_t*
  GetNormalizedLMContextWeights() {
    return m_normalized_lm_context_weights;
  }


  weightmap_t*
  GetLMContextWeights(std::string id) {
    if (!GetLMContextWeights()){
      SPTR<std::map<std::string,float> const> map_sptr = GetContextWeights();
      weightmap_t* map = (weightmap_t*) map_sptr.get();
      return map;
    }
    weightmap_map_t::const_iterator it = (*m_lm_context_weights).find(id);
    if (it != m_lm_context_weights->end()){
      return it->second;
    }
    else{
      SPTR<std::map<std::string,float> const> map_sptr = GetContextWeights();
      weightmap_t* map = (weightmap_t*) map_sptr.get();
      return map;
    }
  }

  weightmap_t*
  GetNormalizedLMContextWeights(std::string id) {
    if (!GetNormalizedLMContextWeights()){
      return GetNormalizedContextWeights();
    }
    weightmap_map_t::const_iterator it = (*m_normalized_lm_context_weights).find(id);
    if (it != m_normalized_lm_context_weights->end()){
      return it->second;
    }
    else{
      return GetNormalizedContextWeights();
    }
  }


  weightmap_t*
  CreateWeightMap(std::string const& spec) {
    weightmap_t* M = new weightmap_t;

    std::vector<std::string> tokens = Tokenize(spec,":");
    for (std::vector<std::string>::iterator it = tokens.begin();
         it != tokens.end(); it++) {
      std::vector<std::string> key_and_value = Tokenize(*it, ",");
      (*M)[key_and_value[0]] = atof(key_and_value[1].c_str());
    }
    return M;
  }

  bool
  SetContextWeights(std::string const& spec) {
    VERBOSE(1,"bool SetContextWeights(std::string const& spec) spec:|" << spec << "|" << std::endl);
    if (m_context_weights) return false;
#ifdef WITH_THREADS
    boost::unique_lock<boost::shared_mutex> lock(m_lock);
#endif
    m_context_weights.reset(CreateWeightMap(spec));

    if (!m_normalized_context_weights){ //if m_normalized_context_weights does not exists, create it
      m_normalized_context_weights = new weightmap_t;
    }

    weightmap_t* map = (weightmap_t*) m_context_weights.get();
    NormalizeWeights(map, m_normalized_context_weights);

    return true;
  }

/*
  bool
  SetContextWeights(std::string const& spec) {
    if (m_context_weights) return false;
    boost::unique_lock<boost::shared_mutex> lock(m_lock);
    SPTR<weightmap_t> M(new weightmap_t);

    // TO DO; This needs to be done with StringPiece.find, not Tokenize
    // PRIORITY: low
    std::vector<std::string> tokens = Tokenize(spec,":");
    for (std::vector<std::string>::iterator it = tokens.begin();
         it != tokens.end(); it++) {
      std::vector<std::string> key_and_value = Tokenize(*it, ",");
      (*M)[key_and_value[0]] = atof(key_and_value[1].c_str());
    }
    m_context_weights = M;
    return true;
  }
*/

  bool
  SetContextWeights(weightmap_t const& w) {
    VERBOSE(1,"bool SetContextWeights(SPTR<weightmap_t const> const& w)" << std::endl);
    if (m_context_weights) return false;
#ifdef WITH_THREADS
    boost::unique_lock<boost::shared_mutex> lock(m_lock);
#endif
    m_context_weights.reset(&w);

    if (!m_normalized_context_weights){ //if m_normalized_context_weights does not exists, create it
      m_normalized_context_weights = new weightmap_t;
    }

    weightmap_t* map = (weightmap_t*) m_context_weights.get();
    NormalizeWeights(map, m_normalized_context_weights);

    return true;
  }

  bool
  SetContextWeights(SPTR<std::map<std::string,float> const> const& w) {
    VERBOSE(1,"bool SetContextWeights(SPTR<std::map<std::string,float> const> const& w)" << std::endl);
    if (m_context_weights) return false;
#ifdef WITH_THREADS
    boost::unique_lock<boost::shared_mutex> lock(m_lock);
#endif
    m_context_weights = w;

    if (!m_normalized_context_weights){ //if m_normalized_context_weights does not exists, create it
      m_normalized_context_weights = new weightmap_t;
    }

    weightmap_t* map = (weightmap_t*) m_context_weights.get();
    NormalizeWeights(map, m_normalized_context_weights);

    return true;
  }


  bool
  SetLMContextWeights(std::string const& spec, std::string const& id) {
VERBOSE(1,"bool SetLMContextWeights(std::string const& spec, std::string const& id)" << std::endl);
VERBOSE(1,"bool SetLMContextWeights(std::string const& spec, std::string const& id) spec:|" << spec << "| id:|" << id << "|" << std::endl);

#ifdef WITH_THREADS
    boost::unique_lock<boost::shared_mutex> lock(m_lock);
#endif
    if (!GetLMContextWeights()){ //if m_lm_context_weight does not exists, create it
      m_lm_context_weights = new weightmap_map_t;
    }
    weightmap_map_t::const_iterator it = m_lm_context_weights->find(id);
    if (it != m_lm_context_weights->end()){ //if the entry associate to "id" already exists, remove it (and then re-create it)
      m_lm_context_weights->erase(id);
    }
    (*m_lm_context_weights)[id] = CreateWeightMap(spec);

    if (!GetNormalizedLMContextWeights()){ //if m_normalized_lm_context_weight does not exists, create it
      m_normalized_lm_context_weights = new weightmap_map_t;
    }
    it = m_normalized_lm_context_weights->find(id);
    if (it != m_normalized_lm_context_weights->end()){ //if the entry associate to "id" already exists, remove it (and then re-create it as empty weightmap_t)
      m_normalized_lm_context_weights->erase(id);
    }
    (*m_normalized_lm_context_weights)[id] = new weightmap_t;
    NormalizeWeights((*m_lm_context_weights)[id], (*m_normalized_lm_context_weights)[id]);

    return true;
  }

  void NormalizeWeights(weightmap_t* in_map, weightmap_t* out_map){
    VERBOSE(1,"void NormalizeWeights(weightmap_t* in_map, weightmap_t* out_map)" << std::endl);
    weightmap_t::const_iterator it;
    float sum=0.0;
    for (it=in_map->begin(); it != in_map->end(); it++){
      sum += fabs(it->second);
    }
    VERBOSE(1,"void NormalizeWeights(weightmap_t* in_map, weightmap_t* out_map) sum:|" << sum << "|" << std::endl);
    for (it=in_map->begin(); it != in_map->end(); it++){
      VERBOSE(1,"void NormalizeWeights(weightmap_t* in_map, weightmap_t* out_map) it->first:|" << it->first << "|" << std::endl);
      VERBOSE(1,"void NormalizeWeights(weightmap_t* in_map, weightmap_t* out_map) it->second:|" << it->second << "|" << std::endl);
      VERBOSE(1,"void NormalizeWeights(weightmap_t* in_map, weightmap_t* out_map) it->second/tmp:|" << it->second/sum << "|" << std::endl);
      (*out_map)[it->first] = (it->second)/sum;
    }
  }

  void print_lm_context_weights(std::string const& id){
    VERBOSE(1,"void print_lm_context_weights(std::string const& id) id:|" << id << "|" << std::endl);
    weightmap_map_t::const_iterator it = m_lm_context_weights->find(id);
    if (it != m_lm_context_weights->end()){ //check whether the entry associate to "id" already existsi
      VERBOSE(1,"m_lm_context_weights id:|" << it->first << "|" << std::endl);
      weightmap_t* _map = it->second;
      if (_map){
        VERBOSE(1,"m_lm_context_weights id:|" << it->first << "| _map:|" << (void*) (_map) << "| size:|" << (_map)->size() << "|" << std::endl);
        weightmap_t::const_iterator it2;
        for (it2 = (*_map).begin(); it2 != (*_map).end(); ++it2){
          VERBOSE(1,"m_lm_context_weights id:|" << it->first << "| key:|" << it2->first << "| value:|" << it2->second << "|" << std::endl);
        }
      }else{
        VERBOSE(1,"printing m_lm_context_weights for id:|" << it->first << "| EMPTY" << std::endl);
      }

    }else{
      VERBOSE(1,"printing m_lm_context_weights does not containd the key id:|" << id << "|" << std::endl);
    }
  }


  void print_context_weights(){
    if (m_context_weights){
      weightmap_t* map = (weightmap_t*) m_context_weights.get();
      VERBOSE(1,"printing m_context_weights m_context_weights:|" << m_context_weights << "| size:|" << map->size() << "|" << std::endl);
      weightmap_t::const_iterator it;
      for (it = map->begin(); it != map->end(); ++it){
        VERBOSE(1,"m_context_weights key:|" << it->first << "| value:|" << it->second << "|" << std::endl);
      }
    }else{
      VERBOSE(1,"printing m_context_weights EMPTY" << std::endl);
    }
  }

  void print_normalized_context_weights(){
    if (m_normalized_context_weights){
      VERBOSE(1,"printing m_normalized_context_weights m_normalized_context_weights:|" << m_normalized_context_weights << "| size:|" << m_normalized_context_weights->size() << "|" << std::endl);
      weightmap_t::const_iterator it;
      for (it = (*m_normalized_context_weights).begin(); it != (*m_normalized_context_weights).end(); ++it){
        VERBOSE(1,"m_normalized_context_weights key:|" << it->first << "| value:|" << it->second << "|" << std::endl);
      }
    }else{
      VERBOSE(1,"printing m_normalized_context_weights EMPTY" << std::endl);
    }
  }

  void print_lm_context_weights(){
    if (m_lm_context_weights){
      VERBOSE(1,"printing m_lm_context_weights m_lm_context_weights:|" << (void*) m_lm_context_weights << "| size:|" << m_lm_context_weights->size() << "|" << std::endl);
      weightmap_map_t::const_iterator it;
      for (it = (*m_lm_context_weights).begin(); it != (*m_lm_context_weights).end(); ++it){
        VERBOSE(1,"m_lm_context_weights id:|" << it->first << "|" << std::endl);
        weightmap_t* _map = it->second;
        if (_map){
          VERBOSE(1,"m_lm_context_weights id:|" << it->first << "| _map:|" << (void*) (_map) << "| size:|" << (_map)->size() << "|" << std::endl);
          weightmap_t::const_iterator it2;
          for (it2 = (*_map).begin(); it2 != (*_map).end(); ++it2){
            VERBOSE(1,"m_lm_context_weights id:|" << it->first << "| key:|" << it2->first << "| value:|" << it2->second << "|" << std::endl);
          }
        }else{
	  VERBOSE(1,"printing m_lm_context_weights for id:|" << it->first << "| EMPTY" << std::endl);
        }
      }
    }else{
      VERBOSE(1,"printing m_lm_context_weights EMPTY" << std::endl);
    }
  }



};

};
