// -*- c++ -*-
// (c) 2008-2010 Ulrich Germann
#include <boost/program_options.hpp>
#include <iomanip>

#include "tpt_typedefs.h"
#include "ug_mm_ttrack.h"
#include "tpt_tokenindex.h"
#include "ug_deptree.h"
#include "ug_corpus_token.h"

using namespace std;
using namespace ugdiss;
namespace po = boost::program_options;

string bname,mtt,mct;
vector<string> range;

typedef L2R_Token<Conll_Sform> Token;

TokenIndex SF,LM,PS,DT;
mmTtrack<Token> MTT;
mmTtrack<SimpleWordId> MCT;
bool sform;
bool have_mtt, have_mct;
bool with_sids;

void 
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",    "print this message")
    ("numbers,n", po::bool_switch(&with_sids), "print sentence ids as first token")
    ("sform,s", po::bool_switch(&sform), "sform only")
    ;
  
  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name")
    ("range", po::value<vector<string> >(&range), "range")
    ;
  po::positional_options_description a;
  a.add("bname",1);
  a.add("range",-1);
  
  po::store(po::command_line_parser(ac,av)
            .options(h.add(o))
            .positional(a)
            .run(),vm);
  po::notify(vm); // IMPORTANT
  if (vm.count("help") || bname.empty())
    {
      cout << "usage:\n\t"
           << av[0] << " track name [<range>]\n"
           << endl;
      cout << o << endl;
      exit(0);
    }
  mtt = bname+".mtt";
  mct = bname+".mct";
}

void 
printRangeMTT(size_t start, size_t stop)
{
  for (;start < stop; start++)
    { 
      size_t i = 0;
      Token const* t = MTT.sntStart(start);
      Token const* e = MTT.sntEnd(start);
      if (with_sids) cout << start << " ";
      for (;t < e; ++t)
        {
#if 0
          uchar const* x = reinterpret_cast<uchar const*>(t);
          cout << *reinterpret_cast<id_type const*>(x) << " ";
          cout << *reinterpret_cast<id_type const*>(x+4) << " ";
          cout << int(*(x+8)) << " ";
          cout << int(*(x+9)) << " ";
          cout << *reinterpret_cast<short const*>(x+10) << endl;
#endif
          if (!sform)
            {
              cout << setw(2) << right << ++i << " ";
	      cout << setw(30) << right << SF[t->sform] << " ";
              cout << setw(4)  << right << PS[t->majpos]   << " ";
              cout << setw(4)  << right << PS[t->minpos]   << " ";
              cout << setw(30) << left  << LM[t->lemma] << " ";
              cout << i+t->parent << " ";
              cout << DT[t->dtype] << endl;
            }
          else cout << SF[t->id()] << " ";
        }
      cout << endl;
    }
}

void 
printRangeMCT(size_t start, size_t stop)
{
  for (;start < stop; start++)
    { 
      SimpleWordId const* t = MCT.sntStart(start);
      SimpleWordId const* e = MCT.sntEnd(start);
      if (with_sids) cout << start << " ";
      while (t < e) cout << SF[(t++)->id()] << " ";
      cout << endl;
    }
}

int 
main(int argc, char*argv[])
{
  interpret_args(argc,argv);
  have_mtt = !access(mtt.c_str(),F_OK);
  have_mct = !have_mtt && !access(mct.c_str(),F_OK);
  if (!have_mtt && !have_mct)
    {
      cerr << "FATAL ERROR: neither " << mtt << " nor " << mct << " exit." << endl;
      exit(1);
    }
  if (have_mtt)
    {
      SF.open(bname+".tdx.sfo"); SF.iniReverseIndex();
      LM.open(bname+".tdx.lem"); LM.iniReverseIndex();
      PS.open(bname+".tdx.pos"); PS.iniReverseIndex();
      DT.open(bname+".tdx.drl"); DT.iniReverseIndex();
      MTT.open(mtt);
    }
  else 
    {
      sform = true;
      SF.open(bname+".tdx"); SF.iniReverseIndex();
      MCT.open(mct);
    }
  
  if (!range.size()) 
    have_mtt ? printRangeMTT(0, MTT.size()) : printRangeMCT(0, MCT.size());
  else
    {
      for (size_t i = 0; i < range.size(); i++)
        {
          istringstream buf(range[i]);
          size_t first,last; uchar c;
          buf>>first;
          if (buf.peek() == '-') buf>>c>>last;
          else                   last = first;
	  if (have_mtt && last < MTT.size()) 
	    printRangeMTT(first,last+1);
	  else if (last < MCT.size())       
	    printRangeMCT(first,last+1);
	}
    }
}
