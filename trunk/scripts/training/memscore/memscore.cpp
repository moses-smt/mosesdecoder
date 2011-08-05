// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#include <iostream>
#include <vector>

#include "phrasetable.h"
#include "scorer.h"

const char *progname;

typedef PhrasePairInfo::AlignmentVector::value_type VP;

bool cmp_counts(const VP &a1, const VP &a2);
int main(int argc, const char *argv[]);

bool cmp_counts(const VP &a1, const VP &a2)
{
  return a1.second < a2.second;
}

int main(int argc, const char *argv[])
{
  progname = argv[0];

  if(argc == 1) {
    std::cerr << "No scorers specified." << std::endl;
    usage();
  }

  MemoryPhraseTable pt;
  PhraseScorerFactory psf(pt);

  typedef std::vector<PhraseScorer *> ScorerList;
  ScorerList scorers;

  for(int argp = 1; argp < argc; ) {
    bool reverse;
    if(!strcmp(argv[argp], "-s"))
      reverse = false;
    else if(!strcmp(argv[argp], "-r"))
      reverse = true;
    else
      usage();

    scorers.push_back(psf.create_scorer(argv, ++argp, reverse));
  }

  pt.load_data(std::cin);
  pt.compute_phrase_statistics();

  for(ScorerList::iterator s = scorers.begin(); s != scorers.end(); ++s)
    (*s)->score_phrases();

  for(PhrasePairCounts::const_iterator it = pt.raw_begin(); it != pt.raw_end(); ++it) {
    PhrasePairInfo ppi(it);
    Phrase src = ppi.get_src();
    Phrase tgt = ppi.get_tgt();
    const PhrasePairInfo::AlignmentVector av = ppi.get_alignments();

    PhraseAlignment alig = std::max_element(av.begin(), av.end(), cmp_counts)->first;

    std::cout << pt.get_src_phrase(src) << " ||| " << pt.get_tgt_phrase(tgt) << " ||| " << alig << " |||";

    for(ScorerList::iterator s = scorers.begin(); s != scorers.end(); ++s)
      std::cout << ' ' << (*s)->get_score(it);
    std::cout << '\n'; // don't use std::endl to avoid flushing
  }
}

void usage()
{
  std::cerr <<	"Usage: " << progname << " <scorer1> <scorer2> ..." << std::endl <<
            "       where each scorer is specified as" << std::endl <<
            "       -s <scorer> <args>         to estimate p(s|t)" << std::endl <<
            "       -r <scorer> <args>         to estimate p(t|s)" << std::endl << std::endl;

  std::cerr <<	"Implemented scorers:" << std::endl;

  const std::vector<String> &v = PhraseScorerFactory::scorer_list();
  std::copy(v.begin(), v.end(), std::ostream_iterator<std::string>(std::cerr, "\n"));

  exit(1);
}
