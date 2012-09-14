
#include <cstring> 
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

#include "_SuffixArraySearchApplicationBase.h"

#include <vector>
#include <iostream>
#include <set>

#ifdef WIN32
#include "WIN32_functions.h"
#else
#include <unistd.h>
#endif

typedef std::set<TextLenType> SentIdSet;
typedef std::map<std::string, SentIdSet> PhraseSetMap;

#undef min

// constants
const size_t MINIMUM_SIZE_TO_KEEP = 10000;     // reduce this to improve memory usage,
// increase for speed
const std::string SEPARATOR       = " ||| ";

const double ALPHA_PLUS_EPS  = -1000.0;        // dummy value
const double ALPHA_MINUS_EPS = -2000.0;        // dummy value

// configuration params
int pfe_filter_limit = 0;               // 0 = don't filter anything based on P(f|e)
bool print_cooc_counts = false;         // add cooc counts to phrase table?
bool print_neglog_significance = false; // add -log(p) to phrase table?
double sig_filter_limit = 0;            // keep phrase pairs with -log(sig) > sig_filter_limit
//    higher = filter-more
bool pef_filter_only = false;           // only filter based on pef

// globals
PhraseSetMap esets;
double p_111 = 0.0;                     // alpha
size_t nremoved_sigfilter = 0;
size_t nremoved_pfefilter = 0;

C_SuffixArraySearchApplicationBase e_sa;
C_SuffixArraySearchApplicationBase f_sa;
int num_lines;

void usage()
{
  std::cerr << "\nFilter phrase table using significance testing as described\n"
            << "in H. Johnson, et al. (2007) Improving Translation Quality\n"
            << "by Discarding Most of the Phrasetable. EMNLP 2007.\n"
            << "\nUsage:\n"
            << "\n  filter-pt -e english.suf-arr -f french.suf-arr\n"
            << "      [-c] [-p] [-l threshold] [-n num] < PHRASE-TABLE > FILTERED-PHRASE-TABLE\n\n"
            << "   [-l threshold] >0.0, a+e, or a-e: keep values that have a -log significance > this\n"
            << "   [-n num      ] 0, 1...: 0=no filtering, >0 sort by P(e|f) and keep the top num elements\n"
            << "   [-c          ] add the cooccurence counts to the phrase table\n"
            << "   [-p          ] add -log(significance) to the phrasetable\n\n";
  exit(1);
}

struct PTEntry {
  PTEntry(const std::string& str, int index);
  std::string f_phrase;
  std::string e_phrase;
  std::string extra;
  std::string scores;
  float pfe;
  int cf;
  int ce;
  int cfe;
  float nlog_pte;
  void set_cooc_stats(int _cef, int _cf, int _ce, float nlp) {
    cfe = _cef;
    cf = _cf;
    ce = _ce;
    nlog_pte = nlp;
  }

};

PTEntry::PTEntry(const std::string& str, int index) :
  cf(0), ce(0), cfe(0), nlog_pte(0.0)
{
  size_t pos = 0;
  std::string::size_type nextPos = str.find(SEPARATOR, pos);
  this->f_phrase = str.substr(pos,nextPos);

  pos = nextPos + SEPARATOR.size();
  nextPos = str.find(SEPARATOR, pos);
  this->e_phrase = str.substr(pos,nextPos-pos);

  pos = nextPos + SEPARATOR.size();
  nextPos = str.find(SEPARATOR, pos);
  this->scores = str.substr(pos,nextPos-pos);

  pos = nextPos + SEPARATOR.size();
  this->extra = str.substr(pos);

  int c = 0;
  std::string::iterator i=scores.begin();
  if (index > 0) {
    for (; i != scores.end(); ++i) {
      if ((*i) == ' ') {
        c++;
        if (c == index) break;
      }
    }
  }
  if (i != scores.end()) {
    ++i;
  }
  char f[24];
  char *fp=f;
  while (i != scores.end() && *i != ' ') {
    *fp++=*i++;
  }
  *fp++=0;

  this->pfe = atof(f);

  // std::cerr << "L: " << f_phrase << " ::: " << e_phrase << " ::: " << scores << " ::: " << pfe << std::endl;
  // std::cerr << "X: " << extra << "\n";
}

struct PfeComparer {
  bool operator()(const PTEntry* a, const PTEntry* b) const {
    return a->pfe > b->pfe;
  }
};

struct NlogSigThresholder {
  NlogSigThresholder(float threshold) : t(threshold) {}
  float t;
  bool operator()(const PTEntry* a) const {
    if (a->nlog_pte < t) {
      delete a;
      return true;
    } else return false;
  }
};

std::ostream& operator << (std::ostream& os, const PTEntry& pp)
{
  //os << pp.f_phrase << " ||| " << pp.e_phrase;
  //os << " ||| " << pp.scores;
  //if (pp.extra.size()>0) os << " ||| " << pp.extra;
  if (print_cooc_counts) os << pp.cfe << " " << pp.cf << " " << pp.ce;
  if (print_neglog_significance) os << " ||| " << pp.nlog_pte;
  return os;
}

void print(int a, int b, int c, int d, float p)
{
  std::cerr << a << "\t" << b << "\t P=" << p << "\n"
            << c << "\t" << d << "\t xf=" << (double)(b)*(double)(c)/(double)(a+1)/(double)(d+1) << "\n\n";
}

// 2x2 (one-sided) Fisher's exact test
// see B. Moore. (2004) On Log Likelihood and the Significance of Rare Events
double fisher_exact(int cfe, int ce, int cf)
{
  assert(cfe <= ce);
  assert(cfe <= cf);

  int a = cfe;
  int b = (cf - cfe);
  int c = (ce - cfe);
  int d = (num_lines - ce - cf + cfe);
  int n = a + b + c + d;

  double cp = exp(lgamma(1+a+c) + lgamma(1+b+d) + lgamma(1+a+b) + lgamma(1+c+d) - lgamma(1+n) - lgamma(1+a) - lgamma(1+b) - lgamma(1+c) - lgamma(1+d));
  double total_p = 0.0;
  int tc = std::min(b,c);
  for (int i=0; i<=tc; i++) {
    total_p += cp;
//      double lg = lgamma(1+a+c) + lgamma(1+b+d) + lgamma(1+a+b) + lgamma(1+c+d) - lgamma(1+n) - lgamma(1+a) - lgamma(1+b) - lgamma(1+c) - lgamma(1+d); double cp = exp(lg);
//      print(a,b,c,d,cp);
    double coef = (double)(b)*(double)(c)/(double)(a+1)/(double)(d+1);
    cp *= coef;
    ++a;
    --c;
    ++d;
    --b;
  }
  return total_p;
}

// input: unordered list of translation options for a single source phrase
void compute_cooc_stats_and_filter(std::vector<PTEntry*>& options)
{
  if (pfe_filter_limit>0 && options.size() > pfe_filter_limit) {
    nremoved_pfefilter += (options.size() - pfe_filter_limit);
    std::nth_element(options.begin(), options.begin()+pfe_filter_limit, options.end(), PfeComparer());
    for (std::vector<PTEntry*>::iterator i=options.begin()+pfe_filter_limit; i != options.end(); ++i)
      delete *i;
    options.erase(options.begin()+pfe_filter_limit,options.end());
  }
  if (pef_filter_only) return;

  SentIdSet fset;
  vector<S_SimplePhraseLocationElement> locations;
  //std::cerr << "Looking up f-phrase: " << options.front()->f_phrase << "\n";

  locations = f_sa.locateExactPhraseInCorpus(options.front()->f_phrase.c_str());
  if(locations.size()==0) {
    cerr<<"No occurrences found!!\n";
  }
  for (vector<S_SimplePhraseLocationElement>::iterator i=locations.begin();
       i != locations.end();
       ++i) {
    fset.insert(i->sentIdInCorpus);
  }
  size_t cf = fset.size();
  for (std::vector<PTEntry*>::iterator i=options.begin(); i != options.end(); ++i) {
    const std::string& e_phrase = (*i)->e_phrase;
    size_t cef=0;
    SentIdSet& eset = esets[(*i)->e_phrase];
    if (eset.empty()) {
      //std::cerr << "Looking up e-phrase: " << e_phrase << "\n";
      vector<S_SimplePhraseLocationElement> locations = e_sa.locateExactPhraseInCorpus(e_phrase.c_str());
      for (vector<S_SimplePhraseLocationElement>::iterator i=locations.begin(); i!= locations.end(); ++i) {
        TextLenType curSentId = i->sentIdInCorpus;
        eset.insert(curSentId);
      }
    }
    size_t ce=eset.size();
    if (ce < cf) {
      for (SentIdSet::iterator i=eset.begin(); i != eset.end(); ++i) {
        if (fset.find(*i) != fset.end()) cef++;
      }
    } else {
      for (SentIdSet::iterator i=fset.begin(); i != fset.end(); ++i) {
        if (eset.find(*i) != eset.end()) cef++;
      }
    }
    double nlp = -log(fisher_exact(cef, cf, ce));
    (*i)->set_cooc_stats(cef, cf, ce, nlp);
    if (ce < MINIMUM_SIZE_TO_KEEP) {
      esets.erase(e_phrase);
    }
  }
  std::vector<PTEntry*>::iterator new_end =
    std::remove_if(options.begin(), options.end(), NlogSigThresholder(sig_filter_limit));
  nremoved_sigfilter += (options.end() - new_end);
  options.erase(new_end,options.end());
}

int main(int argc, char * argv[])
{
  int c;
  const char* efile=0;
  const char* ffile=0;
  int pfe_index = 2;
  while ((c = getopt(argc, argv, "cpf:e:i:n:l:")) != -1) {
    switch (c) {
    case 'e':
      efile = optarg;
      break;
    case 'f':
      ffile = optarg;
      break;
    case 'i':  // index of pfe in phrase table
      pfe_index = atoi(optarg);
      break;
    case 'n':  // keep only the top n entries in phrase table sorted by p(f|e) (0=all)
      pfe_filter_limit = atoi(optarg);
      std::cerr << "P(f|e) filter limit: " << pfe_filter_limit << std::endl;
      break;
    case 'c':
      print_cooc_counts = true;
      break;
    case 'p':
      print_neglog_significance = true;
      break;
    case 'l':
      std::cerr << "-l = " << optarg << "\n";
      if (strcmp(optarg,"a+e") == 0) {
        sig_filter_limit = ALPHA_PLUS_EPS;
      } else if (strcmp(optarg,"a-e") == 0) {
        sig_filter_limit = ALPHA_MINUS_EPS;
      } else {
        char *x;
        sig_filter_limit = strtod(optarg, &x);
      }
      break;
    default:
      usage();
    }
  }
  //-----------------------------------------------------------------------------
  if (optind != argc || ((!efile || !ffile) && !pef_filter_only)) {
    usage();
  }

  //load the indexed corpus with vocabulary(noVoc=false) and with offset(noOffset=false)
  if (!pef_filter_only) {
    e_sa.loadData_forSearch(efile, false, false);
    f_sa.loadData_forSearch(ffile, false, false);
    size_t elines = e_sa.returnTotalSentNumber();
    size_t flines = f_sa.returnTotalSentNumber();
    if (elines != flines) {
      std::cerr << "Number of lines in e-corpus != number of lines in f-corpus!\n";
      usage();
    } else {
      std::cerr << "Training corpus: " << elines << " lines\n";
      num_lines = elines;
    }
    p_111 = -log(fisher_exact(1,1,1));
    std::cerr << "\\alpha = " << p_111 << "\n";
    if (sig_filter_limit == ALPHA_MINUS_EPS) {
      sig_filter_limit = p_111 - 0.001;
    } else if (sig_filter_limit == ALPHA_PLUS_EPS) {
      sig_filter_limit = p_111 + 0.001;
    }
    std::cerr << "Sig filter threshold is = " << sig_filter_limit << "\n";
  } else {
    std::cerr << "Filtering using P(e|f) only. n=" << pfe_filter_limit << std::endl;
  }

  char tmpString[10000];
  std::string prev = "";
  std::vector<PTEntry*> options;
  size_t pt_lines = 0;
  while(!cin.eof()) {
    cin.getline(tmpString,10000,'\n');
    if(++pt_lines%10000==0) {
      std::cerr << ".";
      if(pt_lines%500000==0) std::cerr << "[n:"<<pt_lines<<"]\n";
    }

    if(strlen(tmpString)>0) {
      PTEntry* pp = new PTEntry(tmpString, pfe_index);
      if (prev != pp->f_phrase) {
        prev = pp->f_phrase;

        if (!options.empty()) {  // always true after first line
          compute_cooc_stats_and_filter(options);
        }
        for (std::vector<PTEntry*>::iterator i=options.begin(); i != options.end(); ++i) {
          std::cout << **i << std::endl;
          delete *i;
        }
        options.clear();
        options.push_back(pp);

      } else {
        options.push_back(pp);
      }
      //			  for(int i=0;i<locations.size(); i++){
      //				  cout<<"SentId="<<locations[i].sentIdInCorpus<<" Pos="<<(int)locations[i].posInSentInCorpus<<endl;
      //			  }
    }
  }
  compute_cooc_stats_and_filter(options);
  for (std::vector<PTEntry*>::iterator i=options.begin(); i != options.end(); ++i) {
    std::cout << **i << std::endl;
    delete *i;
  }
  float pfefper = (100.0*(float)nremoved_pfefilter)/(float)pt_lines;
  float sigfper = (100.0*(float)nremoved_sigfilter)/(float)pt_lines;
  std::cerr << "\n\n------------------------------------------------------\n"
            << "  unfiltered phrases pairs: " << pt_lines << "\n"
            << "\n"
            << "     P(f|e) filter [first]: " << nremoved_pfefilter << "   (" << pfefper << "%)\n"
            << "       significance filter: " << nremoved_sigfilter << "   (" << sigfper << "%)\n"
            << "            TOTAL FILTERED: " << (nremoved_pfefilter + nremoved_sigfilter) << "   (" << (sigfper + pfefper) << "%)\n"
            << "\n"
            << "     FILTERED phrase pairs: " << (pt_lines - nremoved_pfefilter - nremoved_sigfilter) << "   (" << (100.0-sigfper - pfefper) << "%)\n"
            << "------------------------------------------------------\n";

  return 0;
}
