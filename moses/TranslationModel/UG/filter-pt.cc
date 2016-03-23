// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// significance filtering for phrase tables as described in
// H. Johnson, et al. (2007) Improving Translation Quality
// by Discarding Most of the Phrasetable. EMNLP 2007.
// Implemented by Marcin Junczys-Dowmunt
// recommended use: -l a+e -n <ttable-limit>
#include <cstring> 
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <sstream>

#include <vector>
#include <iostream>
#include <set>

#include <boost/thread/tss.hpp>
#include <boost/thread.hpp> 
#include <boost/unordered_map.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#ifdef WIN32
#include "WIN32_functions.h"
#else
#include <unistd.h>
#endif

#include "mm/ug_bitext.h"

// constants
const size_t MINIMUM_SIZE_TO_KEEP = 10000;     // increase this to improve memory usage,
// reduce for speed
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
bool hierarchical = false;

double p_111 = 0.0;                     // alpha
size_t pt_lines = 0;
size_t nremoved_sigfilter = 0;
size_t nremoved_pfefilter = 0;

typedef sapt::L2R_Token<sapt::SimpleWordId> Token;
typedef sapt::mmTtrack<Token> ttrack_t;
typedef sapt::mmTSA<Token> tsa_t;
typedef sapt::TokenIndex tind_t;

int num_lines;

boost::mutex in_mutex;
boost::mutex out_mutex;
boost::mutex err_mutex;

typedef size_t TextLenType;

typedef boost::shared_ptr<std::vector<TextLenType> > SentIdSet;

class Cache {
  typedef std::pair<SentIdSet, clock_t> ClockedSet;
  typedef boost::unordered_map<std::string, ClockedSet> ClockedMap;
  
  public:
    
    SentIdSet get(const std::string& phrase) {
      boost::shared_lock<boost::shared_mutex> lock(m_mutex);
      if(m_cont.count(phrase)) {
        ClockedSet& set = m_cont[phrase];
        set.second = clock();
        return set.first;
      }
      return SentIdSet( new SentIdSet::element_type() );
    }
    
    void put(const std::string& phrase, const SentIdSet set) {
      boost::unique_lock<boost::shared_mutex> lock(m_mutex);
      m_cont[phrase] = std::make_pair(set, clock());
    }
    
    static void set_max_cache(size_t max_cache) {
      s_max_cache = max_cache;
    }
    
    void prune() {
      if(s_max_cache > 0) {
        boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
        if(m_cont.size() > s_max_cache) {
          std::vector<clock_t> clocks;
          for(ClockedMap::iterator it = m_cont.begin(); it != m_cont.end(); it++) 
            clocks.push_back(it->second.second);
          
          std::sort(clocks.begin(), clocks.end());
          clock_t out = clocks[m_cont.size() - s_max_cache];
          
          boost::upgrade_to_unique_lock<boost::shared_mutex> uniq_lock(lock);
          for(ClockedMap::iterator it = m_cont.begin(); it != m_cont.end(); it++)
            if(it->second.second < out)
              m_cont.erase(it);
        }
      }
    }
  
  private:
    ClockedMap m_cont;
    boost::shared_mutex m_mutex;
    static size_t s_max_cache;
};

size_t Cache::s_max_cache = 0;

struct SA {
  tind_t V;
  boost::shared_ptr<ttrack_t> T;
  tsa_t I;
  Cache cache;
};

std::vector<boost::shared_ptr<SA> > e_sas;
std::vector<boost::shared_ptr<SA> > f_sas;

#undef min

void usage()
{
  std::cerr << "\nFilter phrase table using significance testing as described\n"
            << "in H. Johnson, et al. (2007) Improving Translation Quality\n"
            << "by Discarding Most of the Phrasetable. EMNLP 2007.\n";
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
  if (nextPos < str.size()) {
    this->scores = str.substr(pos,nextPos-pos);

    pos = nextPos + SEPARATOR.size();
    this->extra = str.substr(pos);
  }
  else {
    this->scores = str.substr(pos,str.size()-pos);
  }

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
  os << pp.f_phrase << " ||| " << pp.e_phrase;
  os << " ||| " << pp.scores;
  if (pp.extra.size()>0) os << " ||| " << pp.extra;
  if (print_cooc_counts) os << " ||| " << pp.cfe << " " << pp.cf << " " << pp.ce;
  if (print_neglog_significance) os << " ||| " << pp.nlog_pte;
  return os;
}

void print(int a, int b, int c, int d, float p)
{
  std::cerr << a << "\t" << b << "\t P=" << p << "\n"
            << c << "\t" << d << "\t xf="
            << (double)(b)*(double)(c)/(double)(a+1)/(double)(d+1) << "\n\n";
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

  double cp = exp(lgamma(1+a+c) + lgamma(1+b+d) + lgamma(1+a+b) + lgamma(1+c+d)
                  - lgamma(1+n) - lgamma(1+a) - lgamma(1+b) - lgamma(1+c)
                  - lgamma(1+d));
  double total_p = 0.0;
  int tc = std::min(b,c);
  for (int i=0; i<=tc; i++) {
    total_p += cp;
    double coef = (double)(b)*(double)(c)/(double)(a+1)/(double)(d+1);
    cp *= coef;
    ++a;
    --c;
    ++d;
    --b;
  }
  return total_p;
}

template <class setType>
void ordered_set_intersect(setType& out, const setType set_1, const setType set_2)
{
    std::set_intersection(set_1->begin(), set_1->end(), set_2->begin(),
                          set_2->end(), inserter(*out, out->begin()) );
}


void lookup_phrase(SentIdSet& ids, const std::string& phrase,
                   tsa_t &my_sa, tind_t &my_v, Cache& cache)
{
    ids = cache.get(phrase);
    if(ids->empty()) {
      
      std::vector<sapt::id_type> snt;
      my_v.fillIdSeq(phrase, snt);

      tsa_t::tree_iterator m(&my_sa);
      size_t k = 0;
      while (k < snt.size() && m.extend(snt[k])) ++k;
      if(k == snt.size()) {
        ids->reserve(m.approxOccurrenceCount()+10);
        sapt::tsa::ArrayEntry I(m.lower_bound(-1));
        char const* stop = m.upper_bound(-1);
        do {
          m.root->readEntry(I.next,I);
          ids->push_back(I.sid);
        } while (I.next != stop);
        
        std::sort(ids->begin(), ids->end());
        SentIdSet::element_type::iterator it =
          std::unique(ids->begin(), ids->end());
        ids->resize(it - ids->begin());
        
        if(ids->size() >= MINIMUM_SIZE_TO_KEEP)
          cache.put(phrase, ids);
      }
    }
}

void lookup_multiple_phrases(SentIdSet& ids, std::vector<std::string> & phrases,
                             tsa_t & my_sa, tind_t &my_v,
                             const std::string & rule, Cache& cache) 
{ 

    if (phrases.size() == 1) {
        lookup_phrase(ids, phrases.front(), my_sa, my_v, cache);
    }
    else {
        SentIdSet main_set( new SentIdSet::element_type() );
        bool first = true;
        SentIdSet first_set( new SentIdSet::element_type() );
        lookup_phrase(first_set, phrases.front(), my_sa, my_v, cache);
        for (std::vector<std::string>::iterator phrase=phrases.begin()+1;
             phrase != phrases.end(); ++phrase) {
            SentIdSet temp_set( new SentIdSet::element_type() );
            lookup_phrase(temp_set, *phrase, my_sa, my_v, cache);
            if (first) {
                ordered_set_intersect(main_set, first_set, temp_set);
                first = false;
            }
            else {
                SentIdSet new_set( new SentIdSet::element_type() );
                ordered_set_intersect(new_set, main_set, temp_set);
                main_set->swap(*new_set);
            }
        }
        ids->swap(*main_set);
    }
}


void find_occurrences(SentIdSet& ids, const std::string& rule,
                      tsa_t& my_sa, tind_t &my_v, Cache& cache)
{
    // we search for hierarchical rules by stripping away NT and looking for terminals sequences
    // if a rule contains multiple sequences of terminals, we intersect their occurrences.
    if (hierarchical) {
        //   std::cerr << "splitting up phrase: " << phrase << "\n";
        int pos = 0;
        int NTStartPos, NTEndPos;
        std::vector<std::string> phrases;
        while (rule.find("] ", pos) < rule.size()) {
            NTStartPos = rule.find("[",pos) - 1; // -1 to cut space before NT
            NTEndPos = rule.find("] ",pos);
            if (NTStartPos < pos) { // no space: NT at start of rule (or two consecutive NTs)
                pos = NTEndPos + 2;
                continue;
            }
            phrases.push_back(rule.substr(pos,NTStartPos-pos));
            pos = NTEndPos + 2;
        }

        NTStartPos = rule.find("[",pos) - 1; // LHS of rule
        if (NTStartPos > pos) {
            phrases.push_back(rule.substr(pos,NTStartPos-pos));
        }

        lookup_multiple_phrases(ids, phrases, my_sa, my_v, rule, cache);
    }
    else {
        lookup_phrase(ids, rule, my_sa, my_v, cache);
    }
}


// input: unordered list of translation options for a single source phrase
void compute_cooc_stats_and_filter(std::vector<PTEntry*>& options)
{
  if (pfe_filter_limit > 0 && options.size() > pfe_filter_limit) {
    nremoved_pfefilter += (options.size() - pfe_filter_limit);
    std::nth_element(options.begin(), options.begin() + pfe_filter_limit,
                     options.end(), PfeComparer());
    for (std::vector<PTEntry*>::iterator i = options.begin() + pfe_filter_limit;
         i != options.end(); ++i)
      delete *i;
    options.erase(options.begin() + pfe_filter_limit,options.end());
  }
  
  if (pef_filter_only)
    return;
  
  if (options.empty())
    return;
  
  size_t cf = 0;
  std::vector<SentIdSet> fsets;
  BOOST_FOREACH(boost::shared_ptr<SA>& f_sa, f_sas) {
    fsets.push_back( boost::shared_ptr<SentIdSet::element_type>(new SentIdSet::element_type()) );
    find_occurrences(fsets.back(), options.front()->f_phrase, f_sa->I, f_sa->V, f_sa->cache);
    cf += fsets.back()->size();
  }
  
  for (std::vector<PTEntry*>::iterator i = options.begin();
       i != options.end(); ++i) {
    const std::string& e_phrase = (*i)->e_phrase;
    
    size_t ce = 0;
    std::vector<SentIdSet> esets;
    BOOST_FOREACH(boost::shared_ptr<SA>& e_sa,  e_sas) {
      esets.push_back( boost::shared_ptr<SentIdSet::element_type>(new SentIdSet::element_type()) );
      find_occurrences(esets.back(), e_phrase, e_sa->I, e_sa->V, e_sa->cache);
      ce += esets.back()->size();
    }
      
    size_t cef = 0;
    for(size_t j = 0; j < fsets.size(); ++j) {
      SentIdSet efset( new SentIdSet::element_type() );
      ordered_set_intersect(efset, fsets[j], esets[j]);
      cef += efset->size();
    }
    
    double nlp = -log(fisher_exact(cef, cf, ce));
    (*i)->set_cooc_stats(cef, cf, ce, nlp);
  }
  
  std::vector<PTEntry*>::iterator new_end =
    std::remove_if(options.begin(), options.end(),
                   NlogSigThresholder(sig_filter_limit));
  nremoved_sigfilter += (options.end() - new_end);
  options.erase(new_end,options.end());
}

void filter_thread(std::istream* in, std::ostream* out, int pfe_index) {
      
  std::vector<std::string> lines;
  std::string prev = "";
  std::vector<PTEntry*> options;
  while(true) {
    {
      boost::mutex::scoped_lock lock(in_mutex);
      if(in->eof())
        break;
      
      lines.clear();
      std::string line;
      while(getline(*in, line) && lines.size() < 500000)
        lines.push_back(line);
    }
    
    std::stringstream out_temp;
    for(std::vector<std::string>::iterator it = lines.begin(); it != lines.end(); it++) {
      size_t tmp_lines = ++pt_lines;
      if(tmp_lines % 10000 == 0) {
        boost::mutex::scoped_lock lock(err_mutex);
        std::cerr << ".";
      
        if(tmp_lines % 500000 == 0)
          std::cerr << "[n:" << tmp_lines << "]\n";
  
        if(tmp_lines % 10000000 == 0) {
          float pfefper = (100.0*(float)nremoved_pfefilter)/(float)pt_lines;
          float sigfper = (100.0*(float)nremoved_sigfilter)/(float)pt_lines;
          std::cerr << "------------------------------------------------------\n"
                    << "  unfiltered phrases pairs: " << pt_lines << "\n"
                    << "\n"
                    << "     P(f|e) filter [first]: " << nremoved_pfefilter << "   (" << pfefper << "%)\n"
                    << "       significance filter: " << nremoved_sigfilter << "   (" << sigfper << "%)\n"
                    << "            TOTAL FILTERED: " << (nremoved_pfefilter + nremoved_sigfilter) << "   (" << (sigfper + pfefper) << "%)\n"
                    << "\n"
                    << "     FILTERED phrase pairs: " << (pt_lines - nremoved_pfefilter - nremoved_sigfilter) << "   (" << (100.0-sigfper - pfefper) << "%)\n"
                    << "------------------------------------------------------\n";
        }
      }
      
      if(pt_lines % 10000 == 0) {
        BOOST_FOREACH(boost::shared_ptr<SA> f_sa, f_sas)
          f_sa->cache.prune();
        BOOST_FOREACH(boost::shared_ptr<SA> e_sa, e_sas)
          e_sa->cache.prune();
      }
      
      if(it->length() > 0) {
        PTEntry* pp = new PTEntry(it->c_str(), pfe_index);
        if (prev != pp->f_phrase) {
          prev = pp->f_phrase;
  
          if (!options.empty()) {  // always true after first line
            compute_cooc_stats_and_filter(options);
          }
          
          for (std::vector<PTEntry*>::iterator i = options.begin();
               i != options.end(); ++i) {
            out_temp << **i << '\n';
            delete *i;
          }
        
          options.clear();
          options.push_back(pp);
  
        } else {
          options.push_back(pp);
        }
      }
    }
    boost::mutex::scoped_lock lock(out_mutex);
    *out << out_temp.str() << std::flush;
  }
  compute_cooc_stats_and_filter(options);
  
  boost::mutex::scoped_lock lock(out_mutex);
  for (std::vector<PTEntry*>::iterator i = options.begin();
       i != options.end(); ++i) {
    *out << **i << '\n';
    delete *i;
  }
  *out << std::flush;
}

namespace po = boost::program_options;

int main(int argc, char * argv[])
{
  bool help;
  std::vector<std::string> efiles;
  std::vector<std::string> ffiles;
  int pfe_index = 2;
  int threads = 1;
  size_t max_cache = 0;
  std::string str_sig_filter_limit;
   
  po::options_description general("General options");
  general.add_options()
    ("english,e", po::value<std::vector<std::string> >(&efiles)->multitoken(),
     "english.suf-arr")
    ("french,f", po::value<std::vector<std::string> >(&ffiles)->multitoken(),
     "french.suf-arr")
    ("pfe-index,i", po::value(&pfe_index)->default_value(2),
     "Index of P(f|e) in phrase table")
    ("pfe-filter-limit,n", po::value(&pfe_filter_limit)->default_value(0),
     "0, 1...: 0=no filtering, >0 sort by P(e|f) and keep the top num elements")
    ("threads,t", po::value(&threads)->default_value(1),
     "number of threads to use")
    ("max-cache,m", po::value(&max_cache)->default_value(0),
     "limit cache to  arg  most recent phrases")
    ("print-cooc,c", po::value(&print_cooc_counts)->zero_tokens()->default_value(false),
     "add the coocurrence counts to the phrase table")
    ("print-significance,p", po::value(&print_neglog_significance)->zero_tokens()->default_value(false),
     "add -log(significance) to the phrase table")
    ("hierarchical,x", po::value(&hierarchical)->zero_tokens()->default_value(false),
     "filter hierarchical rule table")
    ("sig-filter-limit,l", po::value(&str_sig_filter_limit),
     ">0.0, a+e, or a-e: keep values that have a -log significance > this")
    ("help,h", po::value(&help)->zero_tokens()->default_value(false),
     "display this message")
  ;

  po::options_description cmdline_options("Allowed options");
  cmdline_options.add(general);
  po::variables_map vm;
  
  try { 
    po::store(po::command_line_parser(argc,argv).
              options(cmdline_options).run(), vm);
    po::notify(vm);
  }
  catch (std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl << std::endl;
    
    usage();
    std::cout << cmdline_options << std::endl;
    exit(0);
  }
  
  if(vm["help"].as<bool>()) {
    usage();
    std::cout << cmdline_options << std::endl;
    exit(0);
  }
   
  if(vm.count("pfe-filter-limit"))
    std::cerr << "P(f|e) filter limit: " << pfe_filter_limit << std::endl;
  if(vm.count("threads"))
    std::cerr << "Using threads: " << threads << std::endl;  
  if(vm.count("max-cache"))
    std::cerr << "Using max phrases in caches: " << max_cache << std::endl;
    
  if (strcmp(str_sig_filter_limit.c_str(),"a+e") == 0) {
    sig_filter_limit = ALPHA_PLUS_EPS;
  } else if (strcmp(str_sig_filter_limit.c_str(),"a-e") == 0) {
    sig_filter_limit = ALPHA_MINUS_EPS;
  } else {
    char *x;
    sig_filter_limit = strtod(str_sig_filter_limit.c_str(), &x);
    if (sig_filter_limit < 0.0) {
      std::cerr << "Filter limit (-l) must be either 'a+e', 'a-e' or a real number >= 0.0\n";
      usage();
    }
  }
    
  if (sig_filter_limit == 0.0) pef_filter_only = true;
  //-----------------------------------------------------------------------------
  if (optind != argc || ((efiles.empty() || ffiles.empty()) && !pef_filter_only)) {
    usage();
  }
  
  if (!pef_filter_only) {
    size_t elines = 0;
    BOOST_FOREACH(std::string& efile, efiles) {
      e_sas.push_back(boost::shared_ptr<SA>(new SA()));
      e_sas.back()->V.open(efile + ".tdx");
      e_sas.back()->T.reset(new ttrack_t());  
      e_sas.back()->T->open(efile + ".mct");
      e_sas.back()->I.open(efile + ".sfa", e_sas.back()->T);
      elines += e_sas.back()->T->size(); 
    }
    
    size_t flines = 0;
    BOOST_FOREACH(std::string& ffile, ffiles) {
      f_sas.push_back(boost::shared_ptr<SA>(new SA()));
      f_sas.back()->V.open(ffile + ".tdx");
      f_sas.back()->T.reset(new ttrack_t());  
      f_sas.back()->T->open(ffile + ".mct");
      f_sas.back()->I.open(ffile + ".sfa", f_sas.back()->T);
      flines += f_sas.back()->T->size(); 
    }
    
    if (elines != flines) {
      std::cerr << "Number of lines in e-corpus != number of lines in f-corpus!\n";
      usage();
      exit(1);
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

  Cache::set_max_cache(max_cache);
  std::ios_base::sync_with_stdio(false);
  
  boost::thread_group threadGroup;
  for(int i = 0; i < threads; i++) 
    threadGroup.add_thread(new boost::thread(filter_thread, &std::cin, &std::cout, pfe_index));
  threadGroup.join_all();

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
}
