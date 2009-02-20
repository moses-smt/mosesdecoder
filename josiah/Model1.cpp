#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/bind.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include "File.h" 
#include "Model1.h"
#include "Gibbler.h"

namespace Josiah {

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

model1::model1(boost::shared_ptr<external_model1_table> table): _ptable(table) {}

// a half dozen helper utilities to help unravel Moses::Word...
inline const std::string& 
moses_word_to_string(const Moses::Word& w){ return w[0]->GetString(); } // n.b. doesn't work with factors

inline int string_to_id(const std::string& s, const vocabulary& v){
  if (v.right.find(s) == v.right.end()) 
    return -1;
  else
    return v.right.find(s)->second;
}

int moses_word_to_id(const Moses::Word& w, const vocabulary& v){
  return string_to_id(moses_word_to_string(w), v);
}

// generate a vector of ids from a vector of words
typedef std::vector<Moses::Word> moses_words;
inline void moses_words_to_ids(const moses_words::const_iterator& source_begin, 
  const moses_words::const_iterator& source_end,
  const vocabulary& v, std::vector<int>& target){
  target.resize(std::distance(source_begin, source_end));
  std::transform(source_begin, source_end, target.begin(),
    boost::bind(moses_word_to_id, _1, v));

}

inline void moses_words_to_ids(const moses_words& source,
  const vocabulary& v, std::vector<int>& target){
  return moses_words_to_ids(source.begin(), source.end(), v, target);
}

// Moses::Phrase needs some help too..
inline const moses_words& 
moses_phrase_to_words(const Moses::Phrase& source, moses_words& target){
  for (size_t i=0; i<source.GetSize(); ++i){ 
   target.push_back(source.GetWord(i)); 
  } 
  return target;
}

inline const void moses_phrase_to_ids(const Moses::Phrase& source,
  const vocabulary& v, std::vector<int>& target){
  moses_words words;
  moses_words_to_ids(moses_phrase_to_words(source, words), v, target);
}
// ... and now we can do some actual work...

float model1::computeScore(const Sample& sample){
  std::vector<int> f, e;
  moses_words_to_ids(sample.GetSourceWords(), f_vocab(), f);
  moses_words_to_ids(sample.GetTargetWords(), e_vocab(), e);
  return score(f, e);
}

float model1::getSingleUpdateScore(const Sample& sample, 
  const TranslationOption* option, const WordsRange& targetSegment){
  // because this score is a product of sums, and because changing a target 
  // word changes elements of each sum, I don't know of any efficient way to calculate the
  // delta unless some state information is kept between calculations.  Therefore this
  // baseline computation works by calculating the difference between the scores of
  // the sentence both with and without the modified words.  However, this is obviously
  // slow, requiring O(2nm) computations.
  std::vector<int> f, e, e_new_phr, e_with_phr, e_without_phr;

  moses_words_to_ids(sample.GetSourceWords(), f_vocab(), f);
  moses_words_to_ids(sample.GetTargetWords(), e_vocab(), e);
  moses_phrase_to_ids(option->GetTargetPhrase(), e_vocab(), e_new_phr);

  e_with_phr.insert(e_with_phr.end(), e.begin(), e.begin()+targetSegment.GetStartPos());
  e_with_phr.insert(e_with_phr.end(), e_new_phr.begin(), e_new_phr.end());
  e_with_phr.insert(e_with_phr.end(), e.begin()+targetSegment.GetEndPos()+1, e.end());
  
  e_without_phr.insert(e_without_phr.end(), e.begin(), e.begin()+targetSegment.GetStartPos());
  e_without_phr.insert(e_without_phr.end(), e.begin()+targetSegment.GetEndPos()+1, e.end());

  return score(f, e_with_phr) - score(f, e_without_phr);
}

float model1::getPairedUpdateScore(const Sample& sample, 
  const TranslationOption* leftOption, const TranslationOption* rightOption, 
  const WordsRange& targetSegment, const Phrase& targetPhrase){
  // see comment under getSingleUpdateScore re: complexity.
  std::vector<int> f, e, e_new_phr_left, e_new_phr_right, e_with_phr, e_without_phr;

  // TODO: this function and above have some obvious refactoring to do
  moses_words_to_ids(sample.GetSourceWords(), f_vocab(), f);
  moses_words_to_ids(sample.GetTargetWords(), e_vocab(), e);
  moses_phrase_to_ids(leftOption->GetTargetPhrase(), e_vocab(), e_new_phr_left);
  moses_phrase_to_ids(rightOption->GetTargetPhrase(), e_vocab(), e_new_phr_right);

  e_with_phr.insert(e_with_phr.end(), e.begin(), e.begin()+targetSegment.GetStartPos());
  e_with_phr.insert(e_with_phr.end(), e_new_phr_left.begin(), e_new_phr_left.end());
  e_with_phr.insert(e_with_phr.end(), e_new_phr_right.begin(), e_new_phr_right.end());
  e_with_phr.insert(e_with_phr.end(), e.begin()+targetSegment.GetEndPos()+1, e.end());
  
  e_without_phr.insert(e_without_phr.end(), e.begin(), e.begin()+targetSegment.GetStartPos());
  e_without_phr.insert(e_without_phr.end(), e.begin()+targetSegment.GetEndPos()+1, e.end());

  return score(f, e_with_phr) - score(f, e_without_phr);
}

float model1::getFlipUpdateScore(const Sample& s, 
  const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
  const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
  const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment){
  // nothing needed to do here, target words don't change.
  return 0.0;
}

std::string model1::getName() { return "Model 1 -- p(e|f)"; }

model1_inverse::model1_inverse(boost::shared_ptr<external_model1_table> table): _ptable(table) {}

float model1_inverse::computeScore(const Sample& sample){
  std::vector<int> f, e;
  moses_words_to_ids(sample.GetSourceWords(), f_vocab(), f);
  moses_words_to_ids(sample.GetTargetWords(), e_vocab(), e);
  return score(f, e);
}

float model1_inverse::getSingleUpdateScore(const Sample& sample, 
  const TranslationOption* option, const WordsRange& targetSegment){
  std::vector<int> f, e_phr;

  moses_words_to_ids(sample.GetSourceWords(), f_vocab(), f);
  moses_phrase_to_ids(option->GetTargetPhrase(), e_vocab(), e_phr);

  return score(f, e_phr);
}

float model1_inverse::getPairedUpdateScore(const Sample& sample, 
  const TranslationOption* leftOption, const TranslationOption* rightOption, 
  const WordsRange& targetSegment, const Phrase& targetPhrase){
  std::vector<int> f, e_phr_left, e_phr_right;

  // TODO: this function and above have some obvious refactoring to do
  moses_words_to_ids(sample.GetSourceWords(), f_vocab(), f);
  moses_phrase_to_ids(leftOption->GetTargetPhrase(), e_vocab(), e_phr_left);
  moses_phrase_to_ids(rightOption->GetTargetPhrase(), e_vocab(), e_phr_right);

  return score(f, e_phr_left) + score(f, e_phr_right);
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
