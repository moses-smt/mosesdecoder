// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#include <cassert>
#include <cstring>
#include <fstream>

#include "scorer-impl.h"
#include "timestamp.h"
#include "lexdecom.h"

#ifdef ENABLE_CHANNEL_SCORER
#include "channel-scorer.h"
#endif

const std::vector<String> &PhraseScorerFactory::scorer_list()
{
  static std::vector<String> list;
  if(list.size() == 0) {
    list.push_back("ml                              - maximum likelihood score (relative frequency)");
    list.push_back("wittenbell                      - Witten-Bell smoothing");
    list.push_back("absdiscount                     - absolute discounting");
    list.push_back("kndiscount1                     - Knesser-Ney discounting");
    list.push_back("kndiscount3                     - modified Knesser-Ney discounting");
    list.push_back("lexweights <weightfile>         - lexical weights (Koehn et al., NAACL 2003)");
#ifdef ENABLE_CHANNEL_SCORER
    list.push_back("channel <sigma> <srclm> <tgtlm> - channel adaptation");
#endif
    list.push_back("const <constant>                - constant phrase penalty");
    list.push_back("lexdecomp <weightfile>          - lexical decomposition smoothing");
  }

  return list;
}

PhraseScorer *PhraseScorerFactory::create_scorer(const char *argv[], int &argp, bool reverse)
{
  const char *arg = argv[argp++];
  if(arg == NULL)
    usage();

  if(!strcmp(arg, "ml"))
    return MLPhraseScorer::create_scorer(argv, argp, reverse, *this);
  else if(!strcmp(arg, "wittenbell"))
    return WittenBellPhraseScorer::create_scorer(argv, argp, reverse, *this);
  else if(!strcmp(arg, "absdiscount"))
    return AbsoluteDiscountPhraseScorer::create_scorer(argv, argp, reverse, *this);
  else if(!strcmp(arg, "kndiscount1"))
    return KNDiscount1PhraseScorer::create_scorer(argv, argp, reverse, *this);
  else if(!strcmp(arg, "kndiscount3"))
    return KNDiscount3PhraseScorer::create_scorer(argv, argp, reverse, *this);
  else if(!strcmp(arg, "lexweights"))
    return LexicalWeightPhraseScorer::create_scorer(argv, argp, reverse, *this);
#ifdef ENABLE_CHANNEL_SCORER
  else if(!strcmp(arg, "channel"))
    return ChannelAdaptationPhraseScorer::create_scorer(argv, argp, reverse, *this);
#endif
  else if(!strcmp(arg, "const"))
    return ConstantPhraseScorer::create_scorer(argv, argp, reverse, *this);
  else if (!strcmp(arg, "lexdecomp"))
    return LexicalDecompositionPhraseScorer::create_scorer(argv, argp, reverse, *this);
  else {
    std::cerr << "Unknown phrase scorer type: " << arg << std::endl << std::endl;
    usage();
  }
}

PhraseScorer *MLPhraseScorer::create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf)
{
  return new MLPhraseScorer(ptf.get_phrase_table(), reverse);
}

#if 1
void MLPhraseScorer::do_score_phrases() {}
#else
void MLPhraseScorer::do_score_phrases()
{
  Score bla = 0;
  Timestamp t_it;
  for(Count i = 0; i < 200; i++) {
    for(PhraseTable::iterator it = phrase_table_.begin(); it != phrase_table_.end(); ++it) {
      PhrasePairInfo ppair = *it;
      Phrase tgt = ppair.get_tgt();
      bla += bla * ppair.get_count() / phrase_table_.get_tgt_phrase(tgt).get_count();
    }
  }
  std::cerr << "Time for 200 iterations with ML estimation: " << (t_it.elapsed_time() / 1000) << " ms" << std::endl;
  std::cerr << bla << std::endl;
}
#endif

Score MLPhraseScorer::do_get_score(const PhraseTable::const_iterator &it)
{
  PhraseInfo &tgt_phrase = phrase_table_.get_tgt_phrase(it->get_tgt());
  return static_cast<Score>(it->get_count()) / tgt_phrase.get_count();
}

PhraseScorer *WittenBellPhraseScorer::create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf)
{
  return new WittenBellPhraseScorer(ptf.get_phrase_table(), reverse);
}

Score WittenBellPhraseScorer::do_get_score(const PhraseTable::const_iterator &it)
{
  PhraseInfo &tgt_phrase = phrase_table_.get_tgt_phrase(it->get_tgt());
  return static_cast<Score>(it->get_count()) / (tgt_phrase.get_count() + tgt_phrase.get_distinct());
}


PhraseScorer *AbsoluteDiscountPhraseScorer::create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf)
{
  return new AbsoluteDiscountPhraseScorer(ptf.get_phrase_table(), reverse);
}
// p(s|t) = (c(s,t) - beta) / c(t)    <-- absolute discounting

void AbsoluteDiscountPhraseScorer::do_score_phrases()
{
  Count n1 = 0, n2 = 0;

  for(PhraseTable::iterator it = phrase_table_.begin(); it != phrase_table_.end(); ++it) {
    PhrasePairInfo ppinfo = *it;
    Count c = ppinfo.get_count();
    switch(c) {
    case 1:
      n1++;
      break;
    case 2:
      n2++;
    }
  }

  discount_ = static_cast<Score>(n1) / (n1 + 2*n2);
}

inline Score AbsoluteDiscountPhraseScorer::get_discount()
{
  return discount_;
}

Score AbsoluteDiscountPhraseScorer::do_get_score(const PhraseTable::const_iterator &it)
{
  PhraseInfo &tgt_phrase = phrase_table_.get_tgt_phrase(it->get_tgt());
  return (it->get_count() - discount_) / tgt_phrase.get_count();
}

PhraseScorer *KNDiscount1PhraseScorer::create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf)
{
  return new KNDiscount1PhraseScorer(ptf.get_phrase_table(), reverse);
}


void KNDiscount1PhraseScorer::do_score_phrases()
{
  Count n1 = 0, n2 = 0;
  Count total_count = 0;

  for(PhraseTable::iterator it = phrase_table_.begin(); it != phrase_table_.end(); ++it) {
    PhrasePairInfo ppinfo = *it;
    PhraseInfo &tgt_phrase = phrase_table_.get_tgt_phrase(it->get_tgt());
    total_count += tgt_phrase.get_count();

    Count c = ppinfo.get_count();
    switch(c) {
    case 1:
      n1++;
      break;
    case 2:
      n2++;
    }
  }

  discount_ = static_cast<Score>(n1) / (n1 + 2*n2);
  total_count_ = static_cast<Count>(total_count);

}

Score KNDiscount1PhraseScorer::do_get_score(const PhraseTable::const_iterator &it)
{
  PhraseInfo &tgt_phrase = phrase_table_.get_tgt_phrase(it->get_tgt());
  PhraseInfo &src_phrase = phrase_table_.get_src_phrase(it->get_src());
  return ((it->get_count() - discount_) / tgt_phrase.get_count()) + (discount_ * tgt_phrase.get_distinct() / tgt_phrase.get_count())*(src_phrase.get_count() / total_count_);
}

PhraseScorer *KNDiscount3PhraseScorer::create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf)
{
  return new KNDiscount3PhraseScorer(ptf.get_phrase_table(), reverse);
}


void KNDiscount3PhraseScorer::do_score_phrases()
{
  Count n1 = 0, n2 = 0, n3 = 0, n4 = 0;
  Count total_count = 0; //total number of source or target phrases (including repetitions)
  Count total_distinct_n1 = 0; //sum_{s} n1plus(s,*)
  Count total_distinct_n2 = 0;
  Count total_distinct_n3plus = 0;
  Score y;

  for(PhraseTable::iterator it = phrase_table_.begin(); it != phrase_table_.end(); ++it) {
    PhrasePairInfo ppinfo = *it;
    PhraseInfo &src_phrase = phrase_table_.get_src_phrase(it->get_src());
    PhraseInfo &tgt_phrase = phrase_table_.get_tgt_phrase(it->get_tgt());

    total_count += src_phrase.get_count();

    Count c = ppinfo.get_count();
    switch(c) {
    case 1:
      n1++;
      tgt_phrase.inc_n1();
      src_phrase.inc_n1();
      total_distinct_n1++;
      break;
    case 2:
      n2++;
      tgt_phrase.inc_n2();
      src_phrase.inc_n2();
      total_distinct_n2++;
      break;
    case 3:
      n3++;
      tgt_phrase.inc_n3plus();
      src_phrase.inc_n3plus();
      total_distinct_n3plus++;
      break;
    case 4:
      n4++;
      tgt_phrase.inc_n3plus();
      src_phrase.inc_n3plus();
      total_distinct_n3plus++;
    }

  }

  y = (Score)(n1) / (n1 + 2*n2);
  discount1_ = static_cast<Score> (1) - (2)*(y)*(n2 / n1);
  discount2_ = static_cast<Score> (2) - (3)*(y)*(n3 / n2);
  discount3plus_ = static_cast<Score> (3) - (4)*(y)*(n4 / n3);
  total_distinct_n1_ = static_cast<Count>(total_distinct_n1);
  total_distinct_n2_ = static_cast<Count>(total_distinct_n2);
  total_distinct_n3plus_ = static_cast<Count>(total_distinct_n3plus);
}

Score KNDiscount3PhraseScorer::do_get_score(const PhraseTable::const_iterator &it)
{
  PhrasePairInfo ppinfo = *it;
  PhraseInfo &tgt_phrase = phrase_table_.get_tgt_phrase(it->get_tgt());
  PhraseInfo &src_phrase = phrase_table_.get_src_phrase(it->get_src());

  Score norm = (discount1_ * tgt_phrase.get_n1() + discount2_ * tgt_phrase.get_n2() + discount3plus_ * tgt_phrase.get_n3plus()) / tgt_phrase.get_count();
  Count c = ppinfo.get_count();
  switch(c) {
  case 1:
    return ((it->get_count() - discount1_) / tgt_phrase.get_count()) + \
           norm*(src_phrase.get_n1() / total_distinct_n1_);
    break;
  case 2:
    return ((it->get_count() - discount2_) / tgt_phrase.get_count()) + \
           norm*(src_phrase.get_n2() / total_distinct_n2_);
    break;
  default:
    return ((it->get_count() - discount3plus_) / tgt_phrase.get_count()) + \
           norm*(src_phrase.get_n3plus() / total_distinct_n3plus_);
  }
}

PhraseScorer *LexicalWeightPhraseScorer::create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf)
{
  bool overall_max = true;

  if(argv[argp] == NULL)
    usage();

  if(!strcmp(argv[argp], "-AlignmentCount")) {
    overall_max = false;
    argp++;
    if(argv[argp] == NULL)
      usage();
  }

  String lwfile(argv[argp++]);
  return new LexicalWeightPhraseScorer(ptf.get_phrase_table(), reverse, lwfile, overall_max);
}

LexicalWeightPhraseScorer::LexicalWeightPhraseScorer(PhraseTable &pd, bool reverse, const String &weightfile, bool overall_max) :
  PhraseScorer(pd, reverse), overall_max_score_(overall_max), null_word_(PhraseText::index_word("NULL"))
{
  std::ifstream wfile(weightfile.c_str());

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
}

Score LexicalWeightPhraseScorer::get_weight(const String &s_src, const String &s_tgt) const
{
  Count src = PhraseText::index_word(s_src);
  Count tgt = PhraseText::index_word(s_tgt);
  return get_weight(src, tgt);
}

inline Score LexicalWeightPhraseScorer::get_weight(Count src, Count tgt) const
{
  WeightMapType_::const_iterator it = weight_map_.find(std::make_pair(src, tgt));
  if(it == weight_map_.end())
    return 0.00001;		// default value copied from Philipp Koehn's scorer
  return it->second;
}

#if 1
void LexicalWeightPhraseScorer::do_score_phrases() {}
#else
void LexicalWeightPhraseScorer::do_score_phrases()
{
  Score bla = 0;
  Timestamp t_it;
  for(Count i = 0; i < 200; i++) {
    for(PhraseTable::iterator it = phrase_table_.begin(); it != phrase_table_.end(); ++it) {
      PhrasePairInfo ppair = *it;
      Phrase src = ppair.get_src();
      Phrase tgt = ppair.get_tgt();
      bla += bla * get_score(src, tgt);
    }
  }
  std::cerr << "Time for 200 iterations with lexical weights: " << (t_it.elapsed_time() / 1000) << " ms" << std::endl;
  std::cerr << bla << std::endl;
}
#endif

Score LexicalWeightPhraseScorer::do_get_score(const PhraseTable::const_iterator &it)
{
  const Phrase src = it->get_src();
  const Phrase tgt = it->get_tgt();
  const PhraseText &src_phrase = phrase_table_.get_src_phrase(src).get_phrase();
  const PhraseText &tgt_phrase = phrase_table_.get_tgt_phrase(tgt).get_phrase();
  const PhrasePairInfo &ppair = *it;

  Count max_count = 0;

  Score maxlex = 0;
  PhrasePairInfo::AlignmentVector av = ppair.get_alignments();
  for(PhrasePairInfo::AlignmentVector::const_iterator it = av.begin(); it != av.end(); ++it) {
    const PhraseAlignment &alig = it->first;
    const Count alig_cnt = it->second;

    assert(alig.get_source_length() == src_phrase.size() && alig.get_target_length() == tgt_phrase.size());

    if(!overall_max_score_ && alig_cnt < max_count)
      continue;
    max_count = alig_cnt;

    Score lex = 1;
    for(Count s = 0; s < src_phrase.size(); s++) {
      Score factor = 0;
      Count na = 0;
      for(Count t = 0; t < tgt_phrase.size(); t++)
        if(alig.is_aligned(s, t)) {
          const Score w = get_weight(src_phrase[s], tgt_phrase[t]);
          factor += w;
          na++;
        }

      if(na > 0)
        lex *= factor / na;
      else
        lex *= get_weight(src_phrase[s], null_word_);
    }

    if(lex > maxlex)
      maxlex = lex;
  }

  return maxlex;
}

PhraseScorer *ConstantPhraseScorer::create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf)
{
  if(argv[argp] == NULL)
    usage();
  Score c = atof(argv[argp++]);
  return new ConstantPhraseScorer(ptf.get_phrase_table(), reverse, c);
}

Score ConstantPhraseScorer::do_get_score(const PhraseTable::const_iterator &it)
{
  return constant_;
}

