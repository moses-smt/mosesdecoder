// -*- c++ -*-
// test program for dynamic tsas

#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

#include <sys/types.h>
#include <sys/wait.h>

#include "ug_conll_record.h"
#include "tpt_tokenindex.h"
#include "ug_mm_ttrack.h"
#include "tpt_pickler.h"
#include "ug_deptree.h"
#include "moses/TranslationModel/UG/generic/sorting/VectorIndexSorter.h"
#include "ug_im_ttrack.h"
#include "ug_bitext.h"

using namespace std;
using namespace ugdiss;
using namespace Moses;
using namespace boost;
using namespace Moses::bitext;
namespace po=boost::program_options;

typedef L2R_Token<SimpleWordId> L2R;

int main()
{
  SPTR<imBitext<L2R> > bt(new imBitext<L2R>());
  string s1,s2,aln;
  vector<string> S1,S2,ALN;
  while (getline(cin,s1) && getline(cin,s2) && getline(cin,aln))
    {
      S1.push_back(s1);
      S2.push_back(s2);
      ALN.push_back(aln);
    }
  bt = bt->add(S1,S2,ALN);

  TSA<L2R>::tree_iterator m(bt->I2.get());
  m.down();
  do {
    char const* p = m.lower_bound(-1);
    tsa::ArrayEntry I(p);
    do {
      m.root->readEntry(I.next,I);
      L2R const* stop = m.root->getCorpus()->sntEnd(I.sid);
      for (L2R const* t = m.root->getCorpus()->getToken(I); t < stop; ++t)
	cout << (*bt->V2)[t->id()] << " ";
      cout << endl;
    } while (I.next < m.upper_bound(-1));
  } while (m.over());
}
