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

void 
process(TSA<Token> const* idx, TokenIndex const* V)
{
  bitext_t::iter m(idx);
  m.down();
  do
    {
      boost::dynamic_bitset<uint64_t> check(m.root->getCorpus()->size());
      tsa::ArrayEntry I(m.lower_bound(-1));
      do {
	m.root->readEntry(I.next,I);
	check.set(I.sid);
      } while (I.next != m.upper_bound(-1));
      size_t i = check.find_first();      
      cout << i << ":" << "1";
      for (i = check.find_next(i); i < check.size(); i = check.find_next(i))
	cout << " " << i << ":" << "1";
      cout << endl;
    } while (m.over());
}

int main(int argc, char* argv[])
{
  interpret_args(argc,argv);
  B->open(bname, L1, L2);
  process(B->I1.get(), B->V1.get());
  process(B->I2.get(), B->V2.get());
}

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",  "print this message")
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
