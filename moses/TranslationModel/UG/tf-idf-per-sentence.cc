#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/timer/timer.hpp>
#include <algorithm>
#include <iostream>
#include "mm/ug_bitext.h"
#include "generic/file_io/ug_stream.h"
#include <string>
#include <sstream>
#include "mm/ug_bitext_sampler.h"

#include <boost/program_options.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/dynamic_bitset.hpp>

#include <future>
#include <locale.h>

namespace po=boost::program_options;
using namespace Moses;
using namespace sapt;
using namespace std;
using namespace boost;
using boost::timer::cpu_timer;

typedef sapt::L2R_Token<sapt::SimpleWordId> Token;
typedef mmBitext<Token> bitext_t;

size_t topN;
string docname;
string reference_file;
string domain_name;
string bname, L1, L2;
string ifile;

string label_file;
string weighting_scheme="tf-idf"; 

void interpret_args(int ac, char* av[]);
boost::shared_ptr<bitext_t> B(new bitext_t);
size_t numdocs=0;
size_t min_occ=25;
size_t min_len=5;
size_t max_ngram_length=2;
std::string max_cols="1M";
size_t max_columns; // max number of columns 
size_t num_threads;
uint32_t next_row=0;
// threading-related stuff

boost::mutex write_lock;
boost::condition_variable write_ready;
boost::shared_ptr<ug::ThreadPool> tpool;

template<class return_type>
struct Job
{
  typedef boost::packaged_task<return_type> task;
  typedef boost::shared_ptr<task> ptr;
};

vector<ushort> snt_len; // sentence lengths (for sorting)
vector<uint32_t> sid2col; // maps from sentences to column 
vector<uint32_t> col2sid; // maps from colums to sentences 

void 
ini_slen(size_t const start, size_t const stop)
{
  for (size_t a = start; a < stop; ++a)
    snt_len[a] = min(B->T1->sntLen(a), B->T2->sntLen(a));
}

size_t
pre_sort(size_t const start, size_t const stop)
{
  size_t a = start, z = stop;
  for (size_t i = start; i < stop; ++i)
    {
      if (snt_len[i] >= min_len) 
        sid2col[a++] = i;
      else sid2col[--z] = i;
    }
  return a - start;
}

bool 
cmp_length(size_t const a, size_t const b)
{
  return snt_len[a] > snt_len[b];
};

void 
select_sentences(size_t start, size_t const stop, size_t offset, size_t N)
{
  N += start;
  if (N < stop) 
    {
      vector<uint32_t>::iterator o = sid2col.begin();
      nth_element(o + start, o + N, o + stop, cmp_length);
    }
  for (size_t i = start; i < N; ++i)
    {
      col2sid[offset++] = sid2col[i];
      assert(col2sid[offset] != col2sid[offset-1]);
    }
}

void 
fill_snt_len() // fill the vector snt_len 
{ 
  size_t chunk_size = max(1000UL, snt_len.size() / (2 * num_threads));
  vector<boost::unique_future<void> > results1;
  for (size_t i = 0; i < snt_len.size(); i += chunk_size)
    {
      size_t k = min(i + chunk_size, snt_len.size());
      Job<void>::ptr j = boost::make_shared<Job<void>::task>
        (boost::bind(ini_slen, i, k));
      results1.push_back(tpool->add(j));
    }
  BOOST_FOREACH(boost::unique_future<void>& r, results1) r.wait();
}

size_t
filter_by_min_len(vector<size_t>& available, vector<size_t>& order)
// Fill the vector sid2col in a way that filters out sentences that
// are too short. Return total number of available sentences
{
  vector<boost::unique_future<size_t> > results(B->count_docs());
  for (size_t d = 0; d < B->count_docs(); ++d)
    {
      size_t doc_start = B->doc_start(d);
      size_t doc_end   = B->doc_end(d);
      Job<size_t>::ptr j = boost::make_shared<Job<size_t>::task>
        (boost::bind(pre_sort, doc_start, doc_end));
      results[d] = tpool->add(j);
    }
  size_t total_available = 0;
  for (size_t d = 0; d < results.size(); ++d)
    total_available += available[d] = results[d].get();

  iota(order.begin(), order.end(), 0);
  auto cmp = [&available](size_t a, size_t b) 
    {
      return available[a] < available[b];
    };

  sort(order.begin(), order.end(), cmp);
  return total_available;
}

ofstream clabels;
ofstream rlabels;

uint32_t SKIP_THIS = numeric_limits<uint32_t>::max();

struct write_column_labels
{
  void operator()()
  {
    size_t colidx = 0;
    BOOST_FOREACH(uint32_t sid, col2sid)
      {
        clabels << colidx++ << " " << sid << " " << B->sid2docname(sid) << " "
                << B->T1->sntLen(sid) << " " << B->T2->sntLen(sid) << endl;
      }
  }
};

size_t select_sentences()
{
  snt_len.resize(B->T1->size());
  sid2col.resize(B->T1->size());
  vector<size_t> available(B->count_docs());
  vector<size_t> order(B->count_docs()); 
  
  fill_snt_len(); // fill aux vector with min sententence length for each sent. pair
  // filter out short sentences
  size_t total_available = filter_by_min_len(available, order); 
  
  clabels.open((label_file + ".clabels").c_str());
  clabels << "# <column index> <sentence id> <document/domain> "
          << "<snt.len1> <snt.len2> " << endl;
  
  vector<boost::unique_future<void> > results1(order.size());
  size_t remaining = min(max_columns,total_available);
  col2sid.resize(remaining);
  size_t offset = 0;
  char buffer[1000];
  for (size_t i = 0; i < order.size(); ++i)
    {
      size_t N = min(remaining / (order.size() - i), available[order[i]]);
      size_t start = B->doc_start(order[i]);
      size_t stop  = start + N;
      Job<void>::ptr j = boost::make_shared<Job<void>::task>
        (boost::bind(select_sentences, start, stop, offset, N));
      results1[order[i]] = tpool->add(j);
      sprintf(buffer, "# %'15zu out of %'15zu sentences from %s\n", N, 
              B->doc_size(order[i]), B->docid2name(order[i]).c_str());
      clabels << buffer;
      remaining -= N;
      offset += N;
      cerr << buffer;
    }
  col2sid.resize(offset);

  for (size_t i = 0; i < order.size(); ++i)
    results1[i].wait();

  sort(col2sid.begin(), col2sid.end());
  // BOOST_FOREACH(size_t i, col2sid) cerr << i << endl;
  fill(sid2col.begin(), sid2col.end(), SKIP_THIS);
  for (size_t i = 0; i < col2sid.size(); ++i)
    sid2col[col2sid[i]] = i;
  
  // write column labels in a separate thread:
  write_column_labels colwriter;
  tpool->add(colwriter); 
}

// not ready for C++11 yet, so we use boost::thread_specific_ptr
// compile won't let me put these as static members of struct make_row
// don't know why
boost::thread_specific_ptr<vector<uchar> > cnt;
boost::thread_specific_ptr<vector<uint32_t> > doc;
boost::thread_specific_ptr<vector<char> > buf;

uint32_t row_idx = 0;

struct make_row
{

  bitext_t::iter const m;
  uint32_t const row;

  make_row(bitext_t::iter const& _m, uint32_t const _row)
    : m(_m), row(_row) {}
  
  void 
  operator()()  
  {
    TokenIndex& V = m.root == B->I1.get() ? *B->V1 : *B->V2;
    cpu_timer timer;
    if (!cnt.get()) 
      {
	cnt.reset(new vector<uchar>(B->T1->size(),0));
	doc.reset(new vector<uint32_t>);
	doc->reserve(B->T1->size());
	buf.reset(new vector<char>(25*max_columns));
      }
    else
      {
	BOOST_FOREACH(uint32_t sid, *doc)
	  (*cnt)[sid] = 0;
	doc->clear();
      }

    tsa::ArrayEntry I(m.lower_bound(-1));
    size_t wcnt = 0;
    do {
      m.root->readEntry(I.next,I);
      if ((*cnt)[I.sid]++ == 0) doc->push_back(I.sid);
      ++wcnt;
    } while (I.next != m.upper_bound(-1));

    size_t df = doc->size();
    for (size_t i = 0; i < doc->size();)
      {
        uint32_t& sid = (*doc)[i];
        if (sid2col[sid] && sid2col[sid] != SKIP_THIS) ++i;
        else
          {
	    (*cnt)[sid] = 0;
            swap(sid, doc->back());
            doc->pop_back();
          }
      }
    
    // skip tokens that don't occur in enough documents
    if (doc->size() < min_occ)
      return;

    sort(doc->begin(), doc->end());
    char* x = &(*buf)[0];
    if (weighting_scheme == "tf-idf")
      {
	float idf = log(B->T1->size()) - log(df);
	BOOST_FOREACH(uint32_t sid, (*doc))
	  x += sprintf(x, "%u:%.4f ", sid2col[sid],
		       (1 + log((*cnt)[sid])) * idf);
      }
    else if (weighting_scheme == "raw-counts")
      {
	BOOST_FOREACH(uint32_t sid, (*doc))
	  x += sprintf(x, "%u:%d ", sid2col[sid],
		       (*cnt)[sid]);
      }
    else if (weighting_scheme == "binary")
      {
	BOOST_FOREACH(uint32_t sid, (*doc))
	  x += sprintf(x, "%u:1.0 ", sid2col[sid]);
      }
    *(x-1) = '\n';
    
    { // in brackets to create a scope for locking
      boost::unique_lock<boost::mutex> lock(write_lock);
      rlabels << row_idx << '\t'
	      << (&V == B->V1.get() ? L1 : L2) << '\t'
	      << m.str(&V) << '\t'
              << wcnt << '\t'
	      << doc->size() << '\t'
	      << df << endl;
      cerr << row_idx << '\t'
	   << (&V == B->V1.get() ? L1 : L2) << '\t'
	   << m.str(&V) << '\t'
	   << wcnt << " " << doc->size() << " " << df << endl;
      char const* p = &(*buf)[0];
      fwrite(p, (x-p), 1, stdout); 
      ++row_idx;
    }
  }
};


void
process(bitext_t::iter& m, uint32_t& rowctr)
{
  if (m.ca() < min_occ) return;
  make_row r(m, rowctr++);
  tpool->add(r);
  if (m.size() < max_ngram_length && m.down())
    {
      do
	{
	  if (m.ca() >= min_occ) process(m, rowctr);
	} while(m.over());
      m.up();
    }
}

size_t
process(TSA<Token>* idx, uint32_t rowctr)
{
  bitext_t::iter m(idx);
  m.extend(2);
  while (m.ca() >= min_occ)
    {
      process(m, rowctr);
      if (!m.over()) break;
    }
  return rowctr;
}

int main(int argc, char* argv[])
{
  cpu_timer timer;
  setlocale(LC_NUMERIC, "");
  interpret_args(argc,argv);
  cerr << L1 << " -> " << L2 << endl;
  B->open(bname, L1, L2);
  tpool.reset(new ug::ThreadPool(num_threads));

  select_sentences();
  rlabels.open((label_file+".rlabels").c_str());
  rlabels << "# <row> <language> <string> <tf> "
	  << "<df in sample> <df in corpus>" << endl;
  uint32_t rowctr = process(B->I1.get(),0);
  process(B->I2.get(), rowctr);
  tpool->stop();
}

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  num_threads = boost::thread::hardware_concurrency();
  o.add_options()
    ("help,h",  "print this message")
    ("min,m", po::value<size_t>(&min_occ)->default_value(min_occ), 
     "min. occurrences for word to be included")
    ("cols,c", po::value<std::string>(&max_cols)->default_value(max_cols), 
     "max number of columns in the output")
    ("max-ngram-length,n",
     po::value<size_t>(&max_ngram_length)->default_value(max_ngram_length), 
     "max length of ngrams to consider")
    ("threads,t", po::value<size_t>(&num_threads)->default_value(num_threads), 
     "number of threads to use")
    ("weighting-scheme,w",
     po::value<std::string>(&weighting_scheme)->default_value(weighting_scheme), 
     "weighting scheme (tf-idf, binary, or raw-counts)")
    ;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name of corpus")
    ("L1", po::value<string>(&L1), "L1 tag")
    ("L2", po::value<string>(&L2), "L2 tag")
    ("labels", po::value<string>(&label_file), "label file")
    ;

  h.add(o);
  po::positional_options_description a;
  a.add("bname",1);
  a.add("L1",1);
  a.add("L2",1);
  a.add("labels",1);

  try
    {
      po::store(po::command_line_parser(ac,av).options(h).positional(a).run(),
		vm);
      po::notify(vm);
    }
  catch (po::error e)
    {
      std::cout << e.what() << std::endl;
      exit(1);
    }
  
  if (vm.count("help"))
    {
      std::cout << "\nusage:\n\t" << av[0]
		<< " [options] <model file stem> <L1> <L2> <label-file-stem>"
		<< std::endl;
      std::cout << o << std::endl;
      exit(0);
    }

  if(max_cols.back() == 'm') max_cols.back() = 'M';
  if (sscanf(max_cols.c_str(), "%zuM", &max_columns) == 1)
    max_columns *= 1000000;
  else max_columns = atoi(max_cols.c_str());
}
