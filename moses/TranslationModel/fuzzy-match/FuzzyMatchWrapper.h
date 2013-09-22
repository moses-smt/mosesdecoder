//
//  FuzzyMatchWrapper.h
//  moses
//
//  Created by Hieu Hoang on 26/07/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef moses_FuzzyMatchWrapper_h
#define moses_FuzzyMatchWrapper_h

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#endif

#include <fstream>
#include <string>
#include "SuffixArray.h"
#include "Vocabulary.h"
#include "Match.h"
#include "moses/InputType.h"

namespace tmmt
{
class Match;
struct SentenceAlignment;

class FuzzyMatchWrapper
{
public:
  FuzzyMatchWrapper(const std::string &source, const std::string &target, const std::string &alignment);

  std::string Extract(long translationId, const std::string &dirNameStr);

protected:
  // tm-mt
  std::vector< std::vector< tmmt::SentenceAlignment > > targetAndAlignment;
  tmmt::SuffixArray *suffixArray;
  int basic_flag;
  int lsed_flag;
  int refined_flag;
  int length_filter_flag;
  int parse_flag;
  int min_match;
  int multiple_flag;
  int multiple_slack;
  int multiple_max;

  typedef std::map< WORD_ID,std::vector< int > > WordIndex;

  // global cache for word pairs
  std::map< std::pair< WORD_ID, WORD_ID >, unsigned int > m_lsed;
#ifdef WITH_THREADS
  //reader-writer lock
  mutable boost::shared_mutex m_accessLock;
#endif

  void load_corpus( const std::string &fileName, std::vector< std::vector< tmmt::WORD_ID > > &corpus );
  void load_target( const std::string &fileName, std::vector< std::vector< tmmt::SentenceAlignment > > &corpus);
  void load_alignment( const std::string &fileName, std::vector< std::vector< tmmt::SentenceAlignment > > &corpus );

  /** brute force method: compare input to all corpus sentences */
  int basic_fuzzy_match( std::vector< std::vector< tmmt::WORD_ID > > source,
                         std::vector< std::vector< tmmt::WORD_ID > > input ) ;

  /** utlility function: compute length of sentence in characters
   (spaces do not count) */
  unsigned int compute_length( const std::vector< tmmt::WORD_ID > &sentence );
  unsigned int letter_sed( WORD_ID aIdx, WORD_ID bIdx );
  unsigned int sed( const std::vector< WORD_ID > &a, const std::vector< WORD_ID > &b, std::string &best_path, bool use_letter_sed );
  void init_short_matches(WordIndex &wordIndex, long translationId, const std::vector< WORD_ID > &input );
  int short_match_max_length( int input_length );
  void add_short_matches(WordIndex &wordIndex, long translationId, std::vector< Match > &match, const std::vector< WORD_ID > &tm, int input_length, int best_cost );
  std::vector< Match > prune_matches( const std::vector< Match > &match, int best_cost );
  int parse_matches( std::vector< Match > &match, int input_length, int tm_length, int &best_cost );

  void create_extract(int sentenceInd, int cost, const std::vector< WORD_ID > &sourceSentence, const std::vector<SentenceAlignment> &targets, const std::string &inputStr, const std::string  &path, std::ofstream &outputFile);

  std::string ExtractTM(WordIndex &wordIndex, long translationId, const std::string &inputPath);
  Vocabulary &GetVocabulary() {
    return suffixArray->GetVocabulary();
  }

  bool GetLSEDCache(const std::pair< WORD_ID, WORD_ID > &key, unsigned int &value) const;
  void SetLSEDCache(const std::pair< WORD_ID, WORD_ID > &key, const unsigned int &value);

};

}

#endif
