#include <algorithm>
#include <functional>
#include <fstream>
#include <numeric>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/bind.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include "File.h" 
#include "Model1.h"
#include "Gibbler.h"

namespace Josiah {

moses_factor_to_vocab_id::moses_factor_to_vocab_id(const vocabulary& v, 
  const Moses::FactorDirection d, const Moses::FactorType t, 
  Moses::FactorCollection& c){

  for (vocabulary::const_iterator i = v.begin(); i!=v.end(); ++i){
    size_t factor_id = c.AddFactor(d, t, i->right)->GetId(); 
    if (_vocab_id.size() <= factor_id)
      _vocab_id.resize(factor_id+1, -1);
    _vocab_id[factor_id] = i->left;
  }
}  

external_m1_node::external_m1_node(){}

external_m1_node::external_m1_node(FILE* f){ read(f); } 

external_m1_node::external_m1_node(const internal_m1_node& node){
  for (internal_m1_node::const_iterator i=node.begin(); i!=node.end(); ++i){
    keys.push_back(i->first);
    data.push_back(i->second);
  }
}

void external_m1_node::read(FILE* f){
  Moses::fReadVector(f,keys);
  Moses::fReadVector(f,data);
}

size_t external_m1_node::write(FILE* f) const {
  Moses::fWriteVector(f,keys);
  return Moses::fWriteVector(f,data);
}

float external_m1_node::score(int key, int col) const {
  std::vector<int>::const_iterator i = 
    std::lower_bound(keys.begin(),keys.end(),key);
  if (i != keys.end() && *i==key)
    return data[std::distance(keys.begin(),i)][col];
  else
    return 0.0;
}

external_model1_table::external_model1_table(const internal_model1_table& origin, const vocabulary& f_vocab, 
  const vocabulary& e_vocab, const std::string& filename): 
  _f_vocab(f_vocab), _e_vocab(e_vocab) {

  _f=Moses::fOpen(filename.c_str(),"wb");
  
  // write external data (persistent content of _table) using R.Zens utils
  OFF_T default_offset = Moses::fTell(_f);
  external_m1_node().write(_f);
  std::vector<OFF_T> offsets(f_vocab.size(),default_offset); 
 
  for (internal_model1_table::const_iterator i=origin.begin(); i!=origin.end(); ++i){
    offsets[i->first] = Moses::fTell(_f);
    external_m1_node(i->second).write(_f);
  }
  _init_table(offsets.begin(), offsets.end());

  // write internal data using boost::serialize
  std::string ifilename(filename+".i");
  std::ofstream ofs(ifilename.c_str(), std::ios::binary);
  boost::archive::binary_oarchive oa(ofs);
  oa << _f_vocab;
  oa << _e_vocab;
  oa << offsets;
}

external_model1_table::external_model1_table(std::string filename){
  std::string ifilename(filename+".i");
  std::ifstream ifs(ifilename.c_str(), std::ios::binary);
  boost::archive::binary_iarchive ia(ifs);
  ia >> _f_vocab;
  ia >> _e_vocab;
  std::vector<OFF_T> offsets;
  ia >> offsets;
  _f = Moses::fOpen(filename.c_str(), "rb");
  _init_table(offsets.begin(), offsets.end());
}

void external_model1_table::gc(){
  for(cache::iterator i=_cache.begin(); i!=_cache.end(); ++i){
    _table[*i].free();
  }
  _cache.clear();
} 

float external_model1_table::score(const int f, const int e, const int col) {
  _cache.insert(f);
  return _table[f]->score(e,col);
}

const vocabulary& external_model1_table::f_vocab() const { return _f_vocab; }
const vocabulary& external_model1_table::e_vocab() const { return _e_vocab; }

model1::model1(model1_table_handle table, vocab_mapper_handle fmap, vocab_mapper_handle emap):
    _ptable(table), _pfmap(fmap), _pemap(emap),
 m_sp(const_cast<ScoreIndexManager&>(StaticData::Instance().GetScoreIndexManager()),"Model1") {
   vector<float> w(1);
   const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(&m_sp,w);
 }

template <typename C>
void moses_words_to_ids(const moses_factor_to_vocab_id& func, const C& origin, 
  std::vector<int>::iterator dest){
  
  std::transform(origin.begin(), origin.end(), dest, func);
}

void model1::clear_cache_on_change(const Sample& s){
  _sentence_cache.clear();
  _sums_cache.clear();

  _sentence_cache.resize(s.GetSourceWords().size());
  moses_words_to_ids(*_pfmap, s.GetSourceWords(), _sentence_cache.begin());

  // _sums_cache operates only over known source words
  typedef boost::filter_iterator<is_known, std::vector<int>::iterator> known_iter;
  for (known_iter i(_sentence_cache.begin(), _sentence_cache.end());
    i != known_iter(_sentence_cache.end(), _sentence_cache.end()); ++i)
    _sums_cache.push_back(0.0);
  _tmp_sums.resize(_sums_cache.size());

  _ptable->gc();
}

// functor that takes the log of its argument;
// used in logspace product computations of model1
struct to_log{ 
  float operator()(float x) const { return log(std::max(x,MODEL1_SUM_FLOOR)); } 
  typedef float result_type;
  typedef float argument_type;
};
typedef boost::transform_iterator<to_log,std::vector<float>::iterator> log_iter;

float model1::computeScore(const Sample& sample){
  // this function really serves multiple purposes --
  // 1. clear/initialize any sentence-related caching
  clear_cache_on_change(sample);

  // 2. compute sums in each column
  _compute_inner_sums(_sentence_cache.begin(), _sentence_cache.end(),
    boost::make_transform_iterator(sample.GetTargetWords().begin(), *_pemap),
    boost::make_transform_iterator(sample.GetTargetWords().end(), *_pemap),
    _sums_cache.begin());

  // 3. compute product of sums in logspace
  _score_cache = std::accumulate(log_iter(_sums_cache.begin()),  
    log_iter(_sums_cache.end()), 0.0);
  return _score_cache;
}

float model1::getSingleUpdateScore(const Sample& sample, 
  const TranslationOption* option, const WordsRange& targetSegment){

  _compute_inner_sums(_sentence_cache.begin(), _sentence_cache.end(),
    boost::make_transform_iterator(option->GetTargetPhrase().begin(), *_pemap),
    boost::make_transform_iterator(option->GetTargetPhrase().end(), *_pemap),
    _tmp_sums.begin());

  // compute change in sums
  std::transform(_sums_cache.begin(), _sums_cache.end(), _tmp_sums.begin(),
    _tmp_sums.begin(), std::minus<float>());

  return _score_cache - std::accumulate(log_iter(_tmp_sums.begin()),
    log_iter(_tmp_sums.end()), 0.0);
}

float model1::getPairedUpdateScore(const Sample& sample, 
  const TranslationOption* leftOption, const TranslationOption* rightOption, 
  const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment){
  // populate a vector with the e words we are to remove
  std::vector<int> e(std::distance(leftOption->GetTargetPhrase().begin(), leftOption->GetTargetPhrase().end())+
    std::distance(rightOption->GetTargetPhrase().begin(), rightOption->GetTargetPhrase().end()));
  
  moses_words_to_ids(*_pfmap, leftOption->GetTargetPhrase(), e.begin());
  moses_words_to_ids(*_pfmap, rightOption->GetTargetPhrase(), 
    e.begin()+std::distance(leftOption->GetTargetPhrase().begin(), leftOption->GetTargetPhrase().end()));

  _compute_inner_sums(_sentence_cache.begin(), _sentence_cache.end(),
    e.begin(), e.end(), _tmp_sums.begin());

  // compute change in sums
  std::transform(_sums_cache.begin(), _sums_cache.end(), _tmp_sums.begin(),
    _tmp_sums.begin(), std::minus<float>());

  return _score_cache - std::accumulate(log_iter(_tmp_sums.begin()),
    log_iter(_tmp_sums.end()), 0.0);
}

float model1::getPairedUpdateScore(const Sample& sample, 
                                     const TranslationOption* leftOption, const TranslationOption* rightOption, 
                                     const WordsRange& targetSegment, const Phrase& targetPhrase){
  // populate a vector with the e words we are to remove
  std::vector<int> e(std::distance(leftOption->GetTargetPhrase().begin(), leftOption->GetTargetPhrase().end())+
                     std::distance(rightOption->GetTargetPhrase().begin(), rightOption->GetTargetPhrase().end()));
    
  moses_words_to_ids(*_pfmap, leftOption->GetTargetPhrase(), e.begin());
  moses_words_to_ids(*_pfmap, rightOption->GetTargetPhrase(), 
                       e.begin()+std::distance(leftOption->GetTargetPhrase().begin(), leftOption->GetTargetPhrase().end()));
    
  _compute_inner_sums(_sentence_cache.begin(), _sentence_cache.end(),
                        e.begin(), e.end(), _tmp_sums.begin());
    
  // compute change in sums
  std::transform(_sums_cache.begin(), _sums_cache.end(), _tmp_sums.begin(),
                   _tmp_sums.begin(), std::minus<float>());
    
  return _score_cache - std::accumulate(log_iter(_tmp_sums.begin()),
                                          log_iter(_tmp_sums.end()), 0.0);
}
  
float model1::getFlipUpdateScore(const Sample& s, 
  const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
  const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
  const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment){
  // nothing needed to do here, target words don't change.
  return 0.0;
}

std::string model1::getName() { return "Model 1 -- p(e|f)"; }

model1_inverse::model1_inverse(model1_table_handle table, vocab_mapper_handle fmap, vocab_mapper_handle emap):
    _ptable(table), _pfmap(fmap),
_pemap(emap),m_sp(const_cast<ScoreIndexManager&>(StaticData::Instance().GetScoreIndexManager()),"Model1Inverse") {
  vector<float> w(1);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(&m_sp,w);
}


void model1_inverse::clear_cache_on_change(const Sample& s){
  _word_cache.clear();
  _option_cache.clear();
  _sentence_cache.clear();
  _sentence_cache.resize(s.GetSourceWords().size());
  moses_words_to_ids(*_pfmap, s.GetSourceWords(), _sentence_cache.begin());
  _ptable->gc();
}

float model1_inverse::computeScore(const Sample& sample){
  // this function really serves two purposes --
  // 1. clear/initialize any sentence-related caching
  clear_cache_on_change(sample);

  // 2. perform the actual computation
  return score(_sentence_cache.begin(), _sentence_cache.end(), 
    boost::make_transform_iterator(sample.GetTargetWords().begin(), *_pemap),
    boost::make_transform_iterator(sample.GetTargetWords().end(), *_pemap));
}

float model1_inverse::getSingleUpdateScore(const Sample& sample, 
  const TranslationOption* option, const WordsRange& targetSegment){
  if (_option_cache.find(option) == _option_cache.end()) 
    _option_cache[option] = score(_sentence_cache.begin(), _sentence_cache.end(),
    boost::make_transform_iterator(option->GetTargetPhrase().begin(), *_pemap),
    boost::make_transform_iterator(option->GetTargetPhrase().end(), *_pemap));
  return _option_cache[option];    
}

float model1_inverse::getPairedUpdateScore(const Sample& sample, 
  const TranslationOption* leftOption, const TranslationOption* rightOption, 
  const WordsRange& leftTargetSegment,   const WordsRange& rightTargetSegment){
  WordsRange targetSegment(min(leftTargetSegment.GetStartPos(), rightTargetSegment.GetStartPos()),max(leftTargetSegment.GetEndPos(), rightTargetSegment.GetEndPos()));
  
  return getSingleUpdateScore(sample, leftOption, targetSegment) +
         getSingleUpdateScore(sample, rightOption, targetSegment);
}

float model1_inverse::getPairedUpdateScore(const Sample& sample, 
                                             const TranslationOption* leftOption, const TranslationOption* rightOption, 
                                             const WordsRange& targetSegment,   const Phrase& targetPhrase){
    
  return getSingleUpdateScore(sample, leftOption, targetSegment) +
  getSingleUpdateScore(sample, rightOption, targetSegment);
}  
  
  
float model1_inverse::getFlipUpdateScore(const Sample& s, 
  const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
  const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
  const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment){
  // nothing needed to do here, target words don't change.
  return 0.0;
}

std::string model1_inverse::getName() { return "Model 1 inverse -- p(f|e)"; }

} // namespace Josiah
