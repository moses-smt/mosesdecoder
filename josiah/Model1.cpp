#include <algorithm>
#include <functional>
#include <fstream>
#include <numeric>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include "File.h" 
#include "Model1.h"
#include "Gibbler.h"

#define foreach BOOST_FOREACH 

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
    FeatureFunction("Model1"),_ptable(table), _pfmap(fmap), _pemap(emap) {}

template <typename ForwardRange, typename BackInsertIterator>
void _moses_words_to_ids(const moses_factor_to_vocab_id& func, 
  const ForwardRange& origin, const BackInsertIterator& dest){
  
  std::vector<int> unfiltered_ids;
  std::transform(origin.begin(), origin.end(), 
    std::back_inserter(unfiltered_ids), func);
  std::remove_copy_if(unfiltered_ids.begin(), unfiltered_ids.end(),
    dest, is_unknown());  
}

void model1::clear_cache_on_change(const Sample& s){
  if (_source_words == s.GetSourceWords()) 
   return;
  
  _source_words = s.GetSourceWords();

  _source_word_ids.clear();
  _moses_words_to_ids(*_pfmap, s.GetSourceWords(), 
    std::back_inserter(_source_word_ids));

  // _sums_cache operates only over known source words
  _sums.clear();
  _sums.insert(_sums.end(), _source_word_ids.size(), 0.0);

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

  // 2. feature computation
  // convert target words to ids
  std::vector<int> target_word_ids;
  _moses_words_to_ids(*_pemap, sample.GetTargetWords(),  
    std::back_inserter(target_word_ids));

  // compute sums in each column
  _compute_inner_sums(_source_word_ids.begin(), _source_word_ids.end(),
    target_word_ids.begin(), target_word_ids.end(),
    _sums.begin());

  // compute product of sums in logspace
  return std::accumulate(log_iter(_sums.begin()),  
    log_iter(_sums.end()), 0.0);
}

float model1::getSingleUpdateScore(const Sample& sample, 
  const TranslationOption* option, const WordsRange& targetSegment){
  assert(!"Do not call model1::getSingleUpdateScore");
  return 0.0;
}


float model1::getPairedUpdateScore(const Sample& sample, 
                                     const TranslationOption* leftOption, const TranslationOption* rightOption, 
                                     const WordsRange& targetSegment, const Phrase& targetPhrase){
  assert(!"Do not call model1::getPairedUpdateScore");
  return 0.0;
}
  
float model1::getFlipUpdateScore(const Sample& s, 
  const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
  const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
  const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment){
  // nothing needed to do here, target words don't change.
  return 0.0;
}


model1_inverse::model1_inverse(model1_table_handle table, vocab_mapper_handle fmap, vocab_mapper_handle emap):
    FeatureFunction("Model1Inverse"),
 _ptable(table), _pfmap(fmap), _pemap(emap) {}


void model1_inverse::clear_cache_on_change(const Sample& s){
  if (_sourceWords == s.GetSourceWords())
   return;
  _sourceWords = s.GetSourceWords();

  _word_cache.clear();
  _option_cache.clear();
  _sentence_cache.clear();
  _moses_words_to_ids(*_pfmap, s.GetSourceWords(), 
    std::back_inserter(_sentence_cache));
  _ptable->gc();
}

float model1_inverse::computeScore(const Sample& sample){
  // this function really serves two purposes --
  // 1. clear/initialize any sentence-related caching
  clear_cache_on_change(sample);

  // 2. perform the actual computation
  std::vector<int> target_words;
  _moses_words_to_ids(*_pemap, sample.GetTargetWords(),
    std::back_inserter(target_words));
  return score(_sentence_cache.begin(), _sentence_cache.end(), 
    target_words.begin(), target_words.end());
}

float model1_inverse::getSingleUpdateScore(const Sample& sample, 
  const TranslationOption* option, const WordsRange& targetSegment){
  if (_option_cache.find(option) == _option_cache.end()) {
    std::vector<int> target_words;
    _moses_words_to_ids(*_pemap, option->GetTargetPhrase(),
      std::back_inserter(target_words));
    _option_cache[option] = score(_sentence_cache.begin(), _sentence_cache.end(),
      target_words.begin(), target_words.end());
  }    
  return _option_cache[option];    
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

ApproximateModel1::ApproximateModel1(model1_table_handle table, vocab_mapper_handle fmap, vocab_mapper_handle emap):
    model1(table,fmap,emap){}
    
float ApproximateModel1::getImportanceWeight(const Sample& sample) {
  //since the "approximation" is to return 0, this is just the true score
  return model1::computeScore(sample);
}

ApproximateModel1Inverse::ApproximateModel1Inverse(model1_table_handle table, vocab_mapper_handle fmap, vocab_mapper_handle emap):
    model1_inverse(table,fmap,emap){}
    
float ApproximateModel1Inverse::getImportanceWeight(const Sample& sample) {
  //since the "approximation" is to return 0, this is just the true score
  return model1_inverse::computeScore(sample);
}

} // namespace Josiah
