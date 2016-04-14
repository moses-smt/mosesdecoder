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
#include <boost/dynamic_bitset.hpp>

namespace po=boost::program_options;
using namespace Moses;
using namespace sapt;
using namespace std;
using namespace boost;

typedef sapt::L2R_Token<sapt::SimpleWordId> Token;
typedef mmBitext<Token> bitext_t;

size_t topN;
string docname;
string reference_file;
string domain_name;
string bname, L1, L2;
string ifile;

void interpret_args(int ac, char* av[]);
boost::shared_ptr<bitext_t> B(new bitext_t);
size_t numdocs=0;
size_t minocc=25;

void 
process(TSA<Token> const* idx, TokenIndex const* V, ostream& wcnt)
{
  bitext_t::iter m(idx);
  m.down();
  do
    {
      size_t wid = m.getToken(-1)->id();
      cerr << wid << " " << (*V)[wid] << ": "; 
      if (m.getToken(-1)->id() > 2 && m.ca() < minocc) break;
      boost::dynamic_bitset<uint64_t> check(m.root->getCorpus()->size());
      vector<ushort> tf(m.root->getCorpus()->size(),0);
      tsa::ArrayEntry I(m.lower_bound(-1));
      size_t ctr = 0;
      do {
	m.root->readEntry(I.next,I);
	++tf[I.sid]; 
        check.set(I.sid);
        ++ctr;
      } while (I.next != m.upper_bound(-1));

      cerr << ctr << " occurrences in "
           << check.count() << "/" << check.size() 
           << " sentences " << endl;

      float idf = log(check.size()) - log(check.count());
      wcnt << setw(8) << wid << " " << (*V)[wid] << " " 
           << ctr << " " << check.count() << " " << idf << endl;

      size_t i = check.find_first();
      cout << i << ":" << (1 + log(tf[i])) * idf;
      for (i = check.find_next(i); i < check.size(); i = check.find_next(i))
        cout << " " << i << ":" << (1 + log(tf[i])) * idf;
      cout << endl;
    } while (m.over());
}

int main(int argc, char* argv[])
{
  interpret_args(argc,argv);
  B->open(bname, L1, L2);
  cerr << "Ready to go ..." << endl;
  ofstream df1((bname+L1+".wcnt").c_str());
  ofstream df2((bname+L2+".wcnt").c_str());
  process(B->I1.get(), B->V1.get(), df1);
  process(B->I2.get(), B->V2.get(), df2);
}

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",  "print this message")
    ("min,m", po::value<size_t>(&minocc)->default_value(minocc), 
     "min. occurrences for word to be included")
    ;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name of corpus")
    ("L1", po::value<string>(&L1), "L1 tag")
    ("L2", po::value<string>(&L2), "L2 tag")
    ;

  h.add(o);
  po::positional_options_description a;
  a.add("bname",1);
  a.add("L1",1);
  a.add("L2",1);

  po::store(po::command_line_parser(ac,av)
            .options(h)
            .positional(a)
            .run(),vm);
  po::notify(vm);
  if (vm.count("help"))
    {
      std::cout << "\nusage:\n\t" << av[0]
           << " [options] <model file stem> <L1> <L2>" << std::endl;
      std::cout << o << std::endl;
      exit(0);
    }
}
