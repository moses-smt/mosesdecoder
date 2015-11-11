// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// (c) 2008-2010 Ulrich Germann
#include <boost/program_options.hpp>
#include <iomanip>

#include "tpt_typedefs.h"
#include "ug_mm_ttrack.h"
#include "tpt_tokenindex.h"
#include "ug_deptree.h"
#include "ug_corpus_token.h"
#include "tpt_pickler.h"

using namespace std;
using namespace sapt;
namespace po = boost::program_options;

string mamfile;
vector<string> range;

typedef L2R_Token<Conll_Sform> Token;

mmTtrack<char> MAM;
bool with_sids;

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",    "print this message")
    ("numbers,n", po::bool_switch(&with_sids), "print sentence ids as first token")
    ;

  po::options_description h("Hidden Options");
  h.add_options()
    ("mamfile", po::value<string>(&mamfile), "mamfile")
    ("range", po::value<vector<string> >(&range), "range")
    ;
  po::positional_options_description a;
  a.add("mamfile",1);
  a.add("range",-1);

  po::store(po::command_line_parser(ac,av)
            .options(h.add(o))
            .positional(a)
            .run(),vm);
  po::notify(vm); // IMPORTANT
  if (vm.count("help") || mamfile.empty())
    {
      cout << "usage:\n\t"
           << av[0] << " track name [<range>]\n"
           << endl;
      cout << o << endl;
      exit(0);
    }
}

void
printRangeMAM(size_t start, size_t stop)
{
  for (;start < stop; start++)
    {
      // size_t i = 0;
      char const* p = MAM.sntStart(start);
      char const* q = MAM.sntEnd(start);
      if (with_sids) cout << start << " ";
      ushort s,t;
      while (p < q)
	{
	  p = tpt::binread(p,s);
	  p = tpt::binread(p,t);
	  cout << s << "-" << t << " ";
	}
      cout << endl;
    }
}

int
main(int argc, char*argv[])
{
  interpret_args(argc,argv);
  MAM.open(mamfile);
  if (!range.size()) printRangeMAM(0, MAM.size());
  else
    {
      for (size_t i = 0; i < range.size(); i++)
        {
          istringstream buf(range[i]);
          size_t first,last; uchar c;
          buf>>first;
          if (buf.peek() == '-') buf>>c>>last;
          else                   last = first;
	  if (last < MAM.size())
	    printRangeMAM(first,last+1);
	}
    }
}
