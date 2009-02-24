#pragma once
#include <map>
#include <set>
#include <functional>
#include <boost/array.hpp>
#include <boost/bimap.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include "FilePtr.h"
#include "FeatureFunction.h"
#include "Factor.h"

namespace Josiah {

// map between ids and strings
typedef boost::bimap<int,std::string> vocabulary;

// functors to efficiently map moses factor ids to vocabulary ids
class moses_factor_to_vocab_id : public std::unary_function<Moses::Word, int> {
public:
  // initialize functor with the vocabulary to map to, and all of the Moses
  // stuff.  This precomputes a mapping so that we never have to check strings
  // at runtime. 
  moses_factor_to_vocab_id(const vocabulary& v, const Moses::FactorDirection d,
    const Moses::FactorType t, Moses::FactorCollection& c);
  // return the vocab's id for a Moses::Word object  
  inline int operator() (const Moses::Word& w) const {
    if (_vocab_id.size() <= w[0]->GetId())
      return -1;
    return _vocab_id[w[0]->GetId()]; 
  }
private:
  std::vector<int> _vocab_id;  
};



// a fixed-size array of scores
struct m1_scores : public boost::array<float,2> {
  m1_scores(){ assign(0.0); } // needed for operator[] on map, below.
};
typedef std::map<unsigned int,m1_scores > internal_m1_node;
// an in-memory data structure for model 1 scores
typedef std::map<unsigned int,internal_m1_node> internal_model1_table;



/// external data structure holding model 1 scores for single f word
struct external_m1_node {
public:
  external_m1_node();
  /// Construct node from an open file handle. 
  /// Required by FilePtr implementation.
  external_m1_node(FILE* f);
  /// Construct the node from an equivalent internal representation.
  external_m1_node(const internal_m1_node& node);
  /// Read the node from an open file handle.
  /// Overwrites any current contents of the node.
  void read(FILE* f);
  /// Write the node to an open file handle.
  size_t write(FILE* f) const;
  /// Retrieve score associated with a key in column col
  float score(int key, int col) const; 
private:
  std::vector<int> keys;
  std::vector<m1_scores> data;
};



/// a model 1 statistics table that conserves memory use with on-demand loading.
class external_model1_table {
public:
  // construct a table from an in-memory table
  // \param filename a file to be used for external data structures and persistence.
  external_model1_table(const internal_model1_table& origin, const vocabulary& f_vocab, 
    const vocabulary& e_vocab, const std::string& filename);
  /// construct table from an existing external file
  external_model1_table(std::string filename);
  /// reclaim memory used by any recent queries
  void gc();
  /// retrieve a score associated with a particular word pair
  float score(const int f, const int e, const int col); // n.b. non-const due to caching
  const vocabulary& f_vocab() const;
  const vocabulary& e_vocab() const;
private:
  vocabulary _f_vocab;
  vocabulary _e_vocab;
  std::vector<Moses::FilePtr<external_m1_node> > _table;
  FILE* _f;
  
  typedef std::set<int> cache; // track which smart pointers are in memory
  cache _cache;

  // init helper function: create list of smart pointers from list of offsets
  typedef std::vector<OFF_T>::iterator offit;
  inline void _init_table(const offit& begin, const offit& end){
    for (offit i=begin; i!=end; ++i)
      _table.push_back(Moses::FilePtr<external_m1_node>(_f,*i));
  }
};



typedef boost::shared_ptr<external_model1_table> model1_table_handle;
typedef boost::shared_ptr<moses_factor_to_vocab_id> vocab_mapper_handle;

/// feature p(e|f)
class model1 : public FeatureFunction {
public:
  model1(model1_table_handle table, vocab_mapper_handle fmap, vocab_mapper_handle emap);
  /** Compute full score of a sample from scratch **/
  virtual float computeScore(const Sample& sample);
  /** Change in score when updating one segment */
  virtual float getSingleUpdateScore(const Sample& sample, const TranslationOption* option, const WordsRange& targetSegment);
  /** Change in score when updating two segments **/
  virtual float getPairedUpdateScore(const Sample& s, const TranslationOption* leftOption,
                             const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase);
  /** Change in score when flipping */
  virtual float getFlipUpdateScore(const Sample& s, const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
                           const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
                           const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment);
  virtual std::string getName();
  virtual const Moses::ScoreProducer* getScoreProducer()  const {return &m_sp;}
private:
  // used by score() below; n.b. unknown words are assigned an id of -1
  struct is_known{ bool operator()(int x){ return x==-1 ? false : true; } };

  template <typename fwrange>
  float score(const fwrange& f, const fwrange& e){ 
    float total =0.0;
    typedef boost::filter_iterator<is_known, typename fwrange::const_iterator> iter;
    for(iter i(f.begin()); i!=iter(f.end()); ++i){ 
      float sum = 0.0;
      for(iter j(e.begin()); j!=iter(e.end()); ++j){
        sum += _ptable->score(*i, *j, 0);
      }
      total -= log(sum);
    }
    return total; // what happens if total == 0?
  }
  inline const vocabulary& e_vocab() { return _ptable->e_vocab(); }
  inline const vocabulary& f_vocab() { return _ptable->f_vocab(); }
  model1_table_handle _ptable;
  vocab_mapper_handle _pfmap;
  vocab_mapper_handle _pemap;
  FeatureFunctionScoreProducer m_sp;
};



/// feature p(f|e)
class model1_inverse : public FeatureFunction {
public:
  model1_inverse(model1_table_handle table, vocab_mapper_handle fmap, vocab_mapper_handle emap);
  /** Compute full score of a sample from scratch **/
  virtual float computeScore(const Sample& sample);
  /** Change in score when updating one segment */
  virtual float getSingleUpdateScore(const Sample& sample, const TranslationOption* option, const WordsRange& targetSegment);
  /** Change in score when updating two segments **/
  virtual float getPairedUpdateScore(const Sample& s, const TranslationOption* leftOption,
                             const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase);
  /** Change in score when flipping */
  virtual float getFlipUpdateScore(const Sample& s, const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
                           const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
                           const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment);
  virtual std::string getName();
  virtual const Moses::ScoreProducer* getScoreProducer() const {return &m_sp;}
private:
  // used by score() below; n.b. unknown words are assigned an id of -1
  struct is_known{ bool operator()(int x){ return x==-1 ? false : true; } };

  // main scoring logic for a f phrase, e phrase
  template <typename fwiter1, typename fwiter2>
  float score(const fwiter1& f_begin, const fwiter1& f_end, const fwiter2& e_begin, const fwiter2& e_end){ 
    typedef boost::filter_iterator<is_known, fwiter1> known_iter1;
    typedef boost::filter_iterator<is_known, fwiter2> known_iter2;
    float total =0.0;
    for(known_iter2 i(e_begin, e_end); i!=known_iter2(e_end, e_end); ++i){ 
      if (_word_cache.find(*i) == _word_cache.end()) {
        float sum = 0.0;
        for(known_iter1 j(f_begin, f_end); j!=known_iter1(f_end, f_end); ++j){
          sum += _ptable->score(*j, *i, 1);
        }
        _word_cache[*i] = -log(sum);
      }
      total += _word_cache[*i];
    }
    return total; // what happens if total == 0?
  }

  void clear_cache_on_change(const Sample& s);

  model1_table_handle _ptable; // data
  vocab_mapper_handle _pfmap; // maps foreign Moses::Word objects to vocab ids
  vocab_mapper_handle _pemap; // maps english Moses::Word objects to vocab ids
  std::map<int,float> _word_cache; // cached columns for target words
  std::map<const TranslationOption*,float> _option_cache; // cached scores for entire target phrases
  std::vector<int> _sentence_cache; // cached internal rep of source sentence
  FeatureFunctionScoreProducer m_sp;
};

} // namespace Josiah
