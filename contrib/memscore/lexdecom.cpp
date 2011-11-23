/*
 * File:   lexdecom.cpp
 * Author: Felipe Sánchez-Martínez, Universitat d'Alacant <fsanchez@dlsi.ua.es>
 *
 * Created on 2010/01/27
 */

#include "lexdecom.h"
#include "scorer-impl.h"

#include <iostream>
#include <fstream>

PhraseScorer*
LexicalDecompositionPhraseScorer::create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf)
{

  if(argv[argp] == NULL)
    usage();

  String lwfile(argv[argp++]);

  return new LexicalDecompositionPhraseScorer(ptf.get_phrase_table(), reverse, lwfile, argv, argp, ptf);
}

LexicalDecompositionPhraseScorer::LexicalDecompositionPhraseScorer(PhraseTable &pd, bool reverse,
    const String &weightfile, const char *argv[], int &argp,
    const PhraseScorerFactory &ptf) :
  PhraseScorer(pd, reverse)
{

  black_box_scorer = AbsoluteDiscountPhraseScorer::create_scorer(argv, argp, reverse, ptf);

  std::ifstream wfile(weightfile.c_str());

  //Code copied from LexicalWeightPhraseScorer; it should be factored !! -- 2010/01/27

  std::cerr<<"Reading lexical weights from '"<<weightfile<<"' ... ";

  while(!wfile.eof()) {
    if(wfile.fail()) {
      std::cerr << "Problem reading file: " << weightfile << std::endl;
      exit(1);
    }
    String src, tgt;
    Score weight;

    wfile >> src >> tgt >> weight;
    Count src_id = PhraseText::index_word(src);
    Count tgt_id = PhraseText::index_word(tgt);
    weight_map_.insert(std::make_pair(std::make_pair(src_id, tgt_id), weight));
  }

  wfile.close();
  std::cerr<<"done."<<std::endl;
}

Score
LexicalDecompositionPhraseScorer::get_weight(const String &s_src, const String &s_tgt) const
{
  //Code copied from LexicalWeightPhraseScorer; it should be factored !! -- 2010/01/27

  Count src = PhraseText::index_word(s_src);
  Count tgt = PhraseText::index_word(s_tgt);
  return get_weight(src, tgt);
}

inline Score
LexicalDecompositionPhraseScorer::get_weight(Count src, Count tgt) const
{
  //Code copied from LexicalWeightPhraseScorer; it should be factored !! -- 2010/01/27

  WeightMapType_::const_iterator it = weight_map_.find(std::make_pair(src, tgt));
  if(it == weight_map_.end())
    return 0.00001;		// default value copied from Philipp Koehn's scorer
  return it->second;
}

void
LexicalDecompositionPhraseScorer::do_score_phrases()
{

  //Estimate p(J|I) = p(src_len|tgt_len)

  black_box_scorer->score_phrases();

  std::cerr<<"LexicalDecompositionPhraseScorer::do_score_phrases"<<std::endl;

  std::map<unsigned, std::map<unsigned, Count> > count_srclen_tgtlen;
  std::map<unsigned, Count>  total_tgtlen;

  for(PhraseTable::iterator it = phrase_table_.begin(); it != phrase_table_.end(); it++) {
    const PhrasePairInfo &ppair = *it;
    PhraseInfo &src = phrase_table_.get_src_phrase(ppair.get_src());
    PhraseInfo &tgt = phrase_table_.get_tgt_phrase(ppair.get_tgt());
    unsigned src_len = src.get_phrase().size();
    unsigned tgt_len = tgt.get_phrase().size();

    /*//Debug code
    for (unsigned i=0; i<src_len; i++)
      std::cerr<<src.get_phrase().word(i)<<" ";
    std::cerr<<"-> "<<src_len<<" ||| ";
    for (unsigned i=0; i<tgt_len; i++)
      std::cerr<<tgt.get_phrase().word(i)<<" ";
    std::cerr<<"-> "<<tgt_len<<std::endl;
    */

    count_srclen_tgtlen[src_len][tgt_len]+=ppair.get_count();
    total_tgtlen[tgt_len]+=ppair.get_count();
  }

  std::map<unsigned, std::map<unsigned, Count> >::iterator its;
  std::map<unsigned, Count>::iterator itt;

  for (its=count_srclen_tgtlen.begin(); its!=count_srclen_tgtlen.end(); its++) {
    unsigned src_len=its->first;

    for(itt=its->second.begin(); itt!=its->second.end(); itt++) {
      unsigned tgt_len=itt->first;
      Count cnt=itt->second;
      prob_srclen_tgtlen_[src_len][tgt_len] = static_cast<Score>(cnt)/static_cast<Score>(total_tgtlen[tgt_len]);
    }
  }
}

Score
LexicalDecompositionPhraseScorer::get_noisy_or_combination(Count src_word, PhraseInfo &tgt_phrase)
{

  Score sc=1.0;

  unsigned tgt_len=tgt_phrase.get_phrase().size();

  for(unsigned i=0; i<tgt_len; i++) {
    Count tgt_word=tgt_phrase.get_phrase()[i];
    sc *= (1.0 - get_weight(src_word, tgt_word));
  }

  return (1.0 - sc);
}

Score
LexicalDecompositionPhraseScorer::do_get_score(const PhraseTable::const_iterator &it)
{

  /*
    The implementation of this method relies on the asumption that the
     smoothed probabilities produced by AbsoluteDiscountPhraseScorer
     are deficient.
   */

  PhraseInfo &src_phrase = phrase_table_.get_src_phrase(it->get_src());
  PhraseInfo &tgt_phrase = phrase_table_.get_tgt_phrase(it->get_tgt());

  unsigned src_len=src_phrase.get_phrase().size();
  unsigned tgt_len=tgt_phrase.get_phrase().size();

  Score prod=1.0;

  for(unsigned j=0; j<src_len; j++)
    prod *= get_noisy_or_combination(src_phrase.get_phrase()[j], tgt_phrase);

  Score lambda= static_cast<Score>(black_box_scorer->get_discount()) *
                tgt_phrase.get_distinct() / tgt_phrase.get_count();

  Score ret_value = black_box_scorer->get_score(it) + (lambda * prod * prob_srclen_tgtlen_[src_len][tgt_len]);

  /*
  //Debug code
  for (unsigned i=0; i<src_len; i++)
    std::cerr<<src_phrase.get_phrase().word(i)<<" ";
  std::cerr<<" ||| ";
  for (unsigned i=0; i<tgt_len; i++)
    std::cerr<<tgt_phrase.get_phrase().word(i)<<" ";
  std::cerr<<"==> discount: "<<black_box_scorer->get_discount()<<"; black box score: "<<black_box_scorer->get_score(it)
           <<"; lambda: "<<lambda<<"; prod: "
           <<prod<<"; prob_srclen_tgtlen_["<<src_len<<"]["<<tgt_len<<"]: "<<prob_srclen_tgtlen_[src_len][tgt_len]
           <<"; count: "<<it->get_count()<<"; score: "<<ret_value;
  std::cerr<<std::endl;
  */

  return ret_value;
}
