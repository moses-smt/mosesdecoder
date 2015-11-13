// -*- c++ -*-
// Program to extract word cooccurrence counts from a memory-mapped
// word-aligned bitext stores the counts lexicon in the format for
// mm2dTable<uint32_t> (ug_mm_2d_table.h)
//
// (c) 2010-2012 Ulrich Germann

// to do: multi-threading

#include <queue>
#include <iomanip>
#include <vector>
#include <iterator>
#include <sstream>
#include <algorithm>

#include <boost/program_options.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "moses/TranslationModel/UG/generic/program_options/ug_get_options.h"
#include "ug_mm_2d_table.h"
#include "ug_mm_ttrack.h"
#include "ug_corpus_token.h"

using namespace std;
using namespace sapt;
using namespace ugdiss;
using namespace boost::math;

typedef mm2dTable<id_type,id_type,uint32_t,uint32_t> LEX_t;
typedef SimpleWordId Token;

// DECLARATIONS
void interpret_args(int ac, char* av[]);

string swrd,twrd,L1,L2,bname;
TokenIndex V1,V2;
LEX_t LEX;


void
lookup_source(ostream& out, id_type r)
{
  vector<LEX_t::Cell> foo(LEX[r].start,LEX[r].stop);
  sort(foo.begin(),foo.end(),LEX_t::Cell::SortDescendingByValue());
  out << V1[r] << " " << LEX.m1(r) << endl;
  BOOST_FOREACH(LEX_t::Cell const& c, foo)
    {
      out << setw(10) << float(c.val)/LEX.m1(r)       << " "
	  << setw(10) << float(c.val)/LEX.m2(c.id) << " "
	  << V2[c.id] << " " << c.val    << "/" << LEX.m2(c.id) << endl;
    }
}

void
lookup_target(ostream& out, id_type c)
{
  vector<LEX_t::Cell> foo;
  LEX_t::Cell cell;
  for (size_t r = 0; r < LEX.numRows; ++r)
    {
      size_t j = LEX[r][c];
      if (j)
	{
	  cell.id  = r;
	  cell.val = j;
	  foo.push_back(cell);
	}
    }
  sort(foo.begin(),foo.end(),LEX_t::Cell::SortDescendingByValue());
  out << V2[c] << " " << LEX.m2(c) << endl;
  BOOST_FOREACH(LEX_t::Cell const& r, foo)
    {
      out << setw(10) << float(r.val)/LEX.m2(c)       << " "
	  << setw(10) << float(r.val)/LEX.m1(r.id) << " "
	  << V1[r.id] << " " << r.val    << "/" << LEX.m1(r.id) << endl;
    }
}

void
dump(ostream& out)
{
  for (size_t r = 0; r < LEX.numRows; ++r)
    lookup_source(out,r);
  out << endl;
}


int
main(int argc, char* argv[])
{
  interpret_args(argc,argv);
  char c = *bname.rbegin();
  if (c != '/' && c != '.') bname += '.';
  V1.open(bname+L1+".tdx");
  V2.open(bname+L2+".tdx");
  LEX.open(bname+L1+"-"+L2+".lex");

  cout.precision(2);
  id_type swid = V1[swrd];
  id_type twid = V2[twrd];
  if (swid != 1 && twid != 1)
    {
      cout << swrd << " " << twrd << " "
	   << LEX.m1(swid)    << " / "
	   << LEX[swid][twid] << " / "
	   << LEX.m2(twid)    << endl;
    }
  else if (swid != 1)
    lookup_source(cout,swid);
  else if (twid != 1)
    lookup_target(cout,twid);
  else
    dump(cout);
}

void
interpret_args(int ac, char* av[])
{
  namespace po=boost::program_options;
  po::variables_map vm;
  po::options_description o("Options");
  po::options_description h("Hidden Options");
  po::positional_options_description a;

  o.add_options()
    ("help,h",    "print this message")
    ("source,s",po::value<string>(&swrd),"source word")
    ("target,t",po::value<string>(&twrd),"target word")
    ;

  h.add_options()
    ("bname", po::value<string>(&bname), "base name")
    ("L1",    po::value<string>(&L1),"L1 tag")
    ("L2",    po::value<string>(&L2),"L2 tag")
    ;
  a.add("bname",1);
  a.add("L1",1);
  a.add("L2",1);
  get_options(ac,av,h.add(o),a,vm,"cfg");

}


