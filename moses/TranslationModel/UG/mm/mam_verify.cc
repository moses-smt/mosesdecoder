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
using namespace tpt;
using namespace sapt;
namespace po = boost::program_options;

typedef L2R_Token<Conll_Sform> Token;

string bname,L1,L2;
mmTtrack<char> MAM;
mmTtrack<Token> T1,T2;
bool inv;
vector<string> range;
void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",    "print this message")
    ("inv,i", po::bool_switch(&inv), "inverse")
    ;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name")
    ("L1", po::value<string>(&L1), "L1")
    ("L2", po::value<string>(&L2), "L2")
    ("range", po::value<vector<string> >(&range), "range")
    ;
  po::positional_options_description a;
  a.add("bname",1);
  a.add("L1",1);
  a.add("L2",1);
  a.add("range",-1);

  po::store(po::command_line_parser(ac,av)
            .options(h.add(o))
            .positional(a)
            .run(),vm);
  po::notify(vm); // IMPORTANT
  if (vm.count("help") || L2.empty())
    {
      cout << "usage:\n\t"
           << av[0] << " <base name> <L1> <L2> \n"
           << endl;
      cout << o << endl;
      exit(0);
    }
}

size_t
check_range(size_t start, size_t stop)
{
  size_t noAln = 0;
  for (size_t sid = start; sid < stop; ++sid)
    {
      char const* p = MAM.sntStart(sid);
      char const* q = MAM.sntEnd(sid);
      size_t slen = T1.sntLen(sid);
      size_t tlen = T2.sntLen(sid);
      if (p == q) ++noAln;
      ushort s,t;
      while (p < q)
	{
	  p = binread(p,s);
	  p = binread(p,t);
	  if (s >= slen || t >= tlen)
	    {
	      cout << "alignment out of bounds in sentence " << sid << ": "
		   << s << "-" << t << " in " << slen << ":" << tlen << "."
		   << endl;
	      break;
	    }
	}
    }
  return noAln;
}

int
main(int argc, char*argv[])
{
  interpret_args(argc,argv);
  MAM.open(bname+L1+"-"+L2+".mam");
  T1.open(bname+L1+".mct");
  T2.open(bname+L2+".mct");
  if (T1.size() != T2.size() || T1.size() != MAM.size())
    {
      cout << "Track sizes don't match!" << endl;
      exit(1);
    }
  size_t noAln;
  if (!range.size())
    noAln = check_range(0, MAM.size());
  else
    {
      noAln = 0;
      for (size_t i = 0; i < range.size(); i++)
        {
          istringstream buf(range[i]);
          size_t first,last; uchar c;
          buf>>first;
          if (buf.peek() == '-') buf>>c>>last;
          else                   last = first;
	  if (last < MAM.size())
	    noAln += check_range(first,last+1);
	}
    }
  cout << noAln << " sentence pairs without alignment" << endl;
}
