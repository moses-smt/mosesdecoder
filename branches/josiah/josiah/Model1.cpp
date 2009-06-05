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
    SingleValuedFeatureFunction("Model1"),_ptable(table), _pfmap(fmap), _pemap(emap), _sample(NULL) {}

template <typename ForwardRange, typename BackInsertIterator>
void _moses_words_to_ids(const moses_factor_to_vocab_id& func, 
  const ForwardRange& origin, const BackInsertIterator& dest){
  
  std::vector<int> unfiltered_ids;
  std::transform(origin.begin(), origin.end(), 
    std::back_inserter(unfiltered_ids), func);
  std::remove_copy_if(unfiltered_ids.begin(), unfiltered_ids.end(),
    dest, is_unknown());  
}

template <typename ForwardRange, typename BackInsertIterator>
void _moses_words_to_ids_unfiltered(const moses_factor_to_vocab_id& func, 
  const ForwardRange& origin, const BackInsertIterator& dest){
  
  std::vector<int> unfiltered_ids;
  std::transform(origin.begin(), origin.end(), 
    std::back_inserter(unfiltered_ids), func);
  std::copy (unfiltered_ids.begin (), unfiltered_ids.end (), dest);
}


void model1::init(const Sample& sample) {
  _sample = &sample;
  clear_cache(sample);
}

void model1::clear_cache(const Sample& s){
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

float model1::computeScore(){
  // 2. feature computation
  // convert target words to ids
  std::vector<int> target_word_ids;
  _moses_words_to_ids(*_pemap, _sample->GetTargetWords(),  
    std::back_inserter(target_word_ids));

  // compute sums in each column
  _compute_inner_sums(_source_word_ids.begin(), _source_word_ids.end(),
    target_word_ids.begin(), target_word_ids.end(),
    _sums.begin());

  // compute product of sums in logspace
  return std::accumulate(log_iter(_sums.begin()),  
    log_iter(_sums.end()), -(_source_word_ids.size()*log(target_word_ids.size()+1)));
}

float model1::getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap){
  assert(!"Do not call model1::getSingleUpdateScore");
  return 0.0;
}


float model1::getContiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption, 
                                             const TargetGap& gap){
  assert(!"Do not call model1::getDiscontiguousPairedUpdateScore");
  return 0.0;
}

float model1::getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption, 
                                             const TargetGap& leftGap, const TargetGap& rightGap){
  assert(!"Do not call model1::getContiguousPairedUpdateScore");
  return 0.0;
}
  



model1_inverse::model1_inverse(model1_table_handle table, vocab_mapper_handle fmap, vocab_mapper_handle emap):
    SingleValuedFeatureFunction("Model1Inverse"),
 _ptable(table), _pfmap(fmap), _pemap(emap), _sample(NULL) {}

 void model1_inverse::init(const Sample& sample) {
   _sample = &sample;
   clear_cache(sample);
 }
 
void model1_inverse::clear_cache(const Sample& s){
  _sourceWords = s.GetSourceWords();

  _word_cache.clear();
  _option_cache.clear();
  _sentence_cache.clear();
  _moses_words_to_ids(*_pfmap, s.GetSourceWords(), 
    std::back_inserter(_sentence_cache));
  _ptable->gc();
}

float model1_inverse::computeScore(){
  // 2. perform the actual computation
  std::vector<int> target_words;
  _moses_words_to_ids(*_pemap, _sample->GetTargetWords(),
    std::back_inserter(target_words));
  return score(_sentence_cache.begin(), _sentence_cache.end(), 
    target_words.begin(), target_words.end());
}

float model1_inverse::getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) {
  if (_option_cache.find(option) == _option_cache.end()) {
    std::vector<int> target_words;
    _moses_words_to_ids(*_pemap, option->GetTargetPhrase(),
      std::back_inserter(target_words));
    _option_cache[option] = score(_sentence_cache.begin(), _sentence_cache.end(),
      target_words.begin(), target_words.end());
  }    
  return _option_cache[option];    
}


float model1_inverse::getContiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption, 
    const TargetGap& gap) {
    
  return getSingleUpdateScore(leftOption, gap) +
    getSingleUpdateScore(rightOption, gap);
}

float model1_inverse::getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption, 
    const TargetGap& leftGap, const TargetGap& rightGap) {
    
      return getSingleUpdateScore(leftOption, leftGap) +
          getSingleUpdateScore(rightOption, rightGap);
}  
  
ApproximateModel1::ApproximateModel1(model1_table_handle table, vocab_mapper_handle fmap, vocab_mapper_handle emap):
    model1(table,fmap,emap){}

void ApproximateModel1::init(const Sample& sample) {
  model1::init(sample);
  clear_cache(sample);
}

   
void ApproximateModel1::clear_cache(const Sample& s) {
  _source_words = s.GetSourceWords();
  
  _source_word_ids.clear();
  _moses_words_to_ids_unfiltered(*_pfmap, s.GetSourceWords(),
    std::back_inserter(_source_word_ids));

  _option_cache.clear();
  // _sums_cache operates only over known source words
  _sums.clear();
  _sums.insert(_sums.end(), _source_word_ids.size(), 0.0);
 
  _ptable->gc();

}
  
float ApproximateModel1::getImportanceWeight() {
  //since the "approximation" is to return 0, this is just the true score
  return model1::computeScore() - computeScore();
}
  
  
float ApproximateModel1::computeScore() {
  float score = 0.0;
  //cerr << "AM1, In compute score" << endl;
  for (Hypothesis* h =  const_cast<Hypothesis*>(const_cast<Sample*>(_sample)->GetTargetTail()->GetNextHypo()); h; h = const_cast<Hypothesis*>(h->GetNextHypo())) {
    score += getSingleUpdateScore(&(h->GetTranslationOption()), h->GetCurrTargetWordsRange());
  }
  return score;
}

float ApproximateModel1::getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap){
  return getSingleUpdateScore(option,gap.segment);
}  
  
float ApproximateModel1::getSingleUpdateScore(const TranslationOption* option, const WordsRange& segment){
  /*cerr << "Score for option " << *(option->GetSourcePhrase()) << " to " << option->GetTargetPhrase() << " = " ;
  cerr <<  "Option start pos " <<  option->GetStartPos() << ", End pos " << option->GetEndPos() << endl;
  cerr << "Source words ids size = " << _source_word_ids.size() << endl;*/
  if (_option_cache.find(option) == _option_cache.end()) {
    std::vector<int> target_word_ids;
    _moses_words_to_ids_unfiltered(*_pemap, option->GetTargetPhrase(), std::back_inserter(target_word_ids));

    if (_source_word_ids[option->GetStartPos()] == -1 ) {
      _option_cache[option] = MODEL1_LOG_FLOOR;
    }
    else { 
     vector<float> sums(option->GetEndPos() - option->GetStartPos() + 1);
      _compute_inner_sums(_source_word_ids.begin() + option->GetStartPos(), _source_word_ids.begin() + option->GetEndPos() + 1,
                        target_word_ids.begin(), target_word_ids.end(),
                        sums.begin());
      /*cerr << "Sums " ;
      for (size_t i = 0; i < sums.size(); ++i)
       cerr << sums[i] << " ";
      cerr << endl; */
    
      // compute product of sums in logspace
      _option_cache[option] = std::accumulate(log_iter(sums.begin()),  
                         log_iter(sums.end()), 0.0);
    }
  }
  //cerr << "Score " << _option_cache[option] << endl;
  return _option_cache[option];
}
  
  
float ApproximateModel1::getContiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption, 
                                               const TargetGap& gap){
  //cerr << "In get cont score " << endl;
  return getSingleUpdateScore(leftOption, gap) +
  getSingleUpdateScore(rightOption, gap);  
}
  
float ApproximateModel1::getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption, const TranslationOption* rightOption, 
                                                  const TargetGap& leftGap, const TargetGap& rightGap){
  //cerr << "In get discont score " << endl;
  return getSingleUpdateScore(leftOption, leftGap) +
  getSingleUpdateScore(rightOption, rightGap);
}  
  

ApproximateModel1Inverse::ApproximateModel1Inverse(model1_table_handle table, vocab_mapper_handle fmap, vocab_mapper_handle emap):
    model1_inverse(table,fmap,emap){}
    
float ApproximateModel1Inverse::getImportanceWeight() {
  //since the "approximation" is to return 0, this is just the true score
  return model1_inverse::computeScore();
}

  
  
  
  
} // namespace Josiah
