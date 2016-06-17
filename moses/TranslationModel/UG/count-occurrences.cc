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
#include <algorithm>
#include <clocale>
#include <boost/program_options.hpp>
#include <boost/math/distributions/binomial.hpp>

namespace po=boost::program_options;
using namespace Moses;
using namespace sapt;
using namespace std;
using namespace boost;

typedef sapt::L2R_Token<sapt::SimpleWordId> Token;
typedef mmBitext<Token> bitext_t;

string bname, L1, L2;

void interpret_args(int ac, char* av[]);
boost::shared_ptr<bitext_t> B(new bitext_t);

struct sorter_t
{
  bool 
  operator()(bitext_t::iter const& A, bitext_t::iter const& B) const
  {
    if (round(A.ca()) != round(B.ca())) 
      return A.ca() < B.ca();
    
    size_t i=0;
    for (;i < A.size() && i < B.size(); ++i)
      {
	Token const* a = A.getToken(i);
	Token const* b = B.getToken(i);
	if (a->id() != b->id()) return a->id() > b->id();
      }
    if (i == A.size()) return i < B.size();
    return false;
  }
};


int main(int argc, char* argv[])
{
  interpret_args(argc,argv);
  setlocale(LC_NUMERIC, "");
  B->open(bname, L1, L2);
  bitext_t::iter m(B->I1.get());
  vector<bitext_t::iter> heap; heap.reserve(100000000);
  heap.push_back(m);
  sorter_t sorter;
  int ctr=0;
  while (heap.size())
    {
      pop_heap(heap.begin(),heap.end(),sorter);
      m = heap.back();
      heap.pop_back();
      if (m.size()) 
	{
	  size_t c = round(m.ca());
	  char buf[1000];
	  sprintf(buf,"%'8d. %'15d %s", ++ctr, c , m.str(B->V1.get()).c_str());
	  cout << buf << endl;
	}
      if (m.down()) 
	{
	  do { 
	    if (m.ca() < 1000) continue;
	    heap.push_back(m);
	    push_heap(heap.begin(),heap.end(),sorter);
	  } while (m.over());
	}
    }
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
