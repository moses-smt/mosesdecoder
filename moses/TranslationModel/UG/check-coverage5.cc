// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-

// read a text from stdin, report percentage of n-grams covered

#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <iostream>
#include "mm/ug_bitext.h"
#include "generic/file_io/ug_stream.h"
#include <string>
#include <sstream>
#include "mm/ug_bitext_sampler.h"

#include <boost/program_options.hpp>
#include <boost/math/distributions/binomial.hpp>

// #include "LSA.h"

namespace po=boost::program_options;
using namespace Moses;
using namespace sapt;
using namespace std;
using namespace boost;

typedef sapt::L2R_Token<sapt::SimpleWordId> Token;
typedef mmTtrack<Token> ttrack_t;

size_t ngram_size;
size_t verbosity;
string bname;
vector<string> ifiles;

void interpret_args(int ac, char* av[]);


void
dump(mmTSA<Token>::tree_iterator& m, TokenIndex& V)
{
  if (m.size()) cout << m.str(NULL) << endl;
  if (m.size()) cout << m.str(&V) << endl;
  if (m.down())
    {
      do { dump(m, V); } while (m.over());
      m.up();
    }
}

int
main(int argc, char* argv[])
{
  interpret_args(argc,argv);
  TokenIndex V;
  V.open(bname+".tdx"); V.setDynamic(true); V.iniReverseIndex();
  boost::shared_ptr<mmTtrack<Token> > T(new mmTtrack<Token>);
  T->open(bname+".mct");
  mmTSA<Token> I; I.open(bname+".sfa", T);

  string line;
  BOOST_FOREACH(string const& file, ifiles)
    {
      size_t total_ngrams=0;
      float matched_ngrams=0;
      ifstream in(file.c_str());
      while(getline(in,line))
        {
          // cout << line << endl;
          vector<id_type> snt;
          V.fillIdSeq(line,snt);
          if (snt.size() < ngram_size) continue;
          total_ngrams += snt.size() - ngram_size + 1;
          for (size_t i = 0; i + ngram_size <= snt.size(); ++i)
            // for (size_t i = 0; i < snt.size(); ++i)
            {
              mmTSA<Token>::tree_iterator m(&I);
              size_t stop = min(snt.size(), i+ngram_size);
              size_t k = i; 
              while (k < stop && m.extend(snt[k])) ++k;
              if (verbosity) cout << i << " " << k-i << " " << m.str(&V) << endl;
              if (k - i == ngram_size)
                ++matched_ngrams;
            }
        }
      printf ("%5.1f%% matched %zu-grams (%.0f/%zu): %s\n",
              (100 * matched_ngrams / total_ngrams), ngram_size,
              matched_ngrams, total_ngrams, file.c_str());
    }
}

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()

    ("help,h",  "print this message")
    ("ngram-size,n", po::value<size_t>(&ngram_size)->default_value(5),
     "sample size")
    ("verbose,v", po::value<size_t>(&verbosity)->default_value(0),
     "verbosity")
    ;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name of corpus")
    ("ifiles", po::value<vector<string> >(&ifiles), "input files")
    ;

  h.add(o);
  po::positional_options_description a;
  a.add("bname",1);
  a.add("ifiles",-1);

  po::store(po::command_line_parser(ac,av)
            .options(h)
            .positional(a)
            .run(),vm);
  po::notify(vm);
  if (vm.count("help"))
    {
      std::cout << "\nusage:\n\t" << av[0]
                << " [options] <model file stem>" << std::endl;
      std::cout << o << std::endl;
      exit(0);
    }
}
