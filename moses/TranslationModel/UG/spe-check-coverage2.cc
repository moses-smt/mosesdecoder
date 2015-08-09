#include "mmsapt.h"
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "moses/TranslationModel/UG/generic/program_options/ug_splice_arglist.h"
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <iostream>

using namespace Moses;
using namespace sapt;
using namespace std;
using namespace boost;

typedef L2R_Token<SimpleWordId> Token;
typedef mmBitext<Token> mmbitext;
typedef imBitext<Token> imbitext;
typedef Bitext<Token>::iter iter;

mmbitext bg;

void
show(ostream& out, iter& f)
{
  iter b(bg.I2.get(),f.getToken(0),f.size());
  if (b.size() == f.size())
    out << setw(12) << int(round(b.approxOccurrenceCount()));
  else
    out << string(12,' ');
  out << " " << setw(5) <<  int(round(f.approxOccurrenceCount())) << " ";
  out << f.str(bg.V1.get()) << endl;
}


void
dump(ostream& out, iter& f)
{
  float cnt = f.size() ? f.approxOccurrenceCount() : 0;
  if (f.down())
    {
      cnt = f.approxOccurrenceCount();
      do { dump(out,f); }
      while (f.over());
      f.up();
    }
  if (f.size() && cnt < f.approxOccurrenceCount() && f.approxOccurrenceCount() > 1)
    show(out,f);
}


void
read_data(string fname, vector<string>& dest)
{
  ifstream in(fname.c_str());
  string line;
  while (getline(in,line)) dest.push_back(line);
  in.close();
}

int main(int argc, char* argv[])
{
  bg.open(argv[1],argv[2],argv[3]);
  sptr<imbitext> fg(new imbitext(bg.V1,bg.V2));
  vector<string> src,trg,aln;
  read_data(argv[4],src);
  read_data(argv[5],trg);
  read_data(argv[6],aln);
  fg = fg->add(src,trg,aln);
  iter mfg(fg->I1.get());
  dump(cout,mfg);
  exit(0);
}



