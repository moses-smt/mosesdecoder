#include "mmsapt.h"
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "moses/TranslationModel/UG/generic/program_options/ug_splice_arglist.h"
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace Moses;
using namespace sapt;
using namespace std;
using namespace boost;

typedef L2R_Token<SimpleWordId> Token;
typedef mmBitext<Token> mmbitext;
typedef imBitext<Token> imbitext;
typedef Bitext<Token>::iter iter;

mmbitext bg;
vector<string> src,trg,aln;

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

void
show_snt(ostream& out, TokenIndex const& V, vector<Token> const& snt,
	 vector<vector<int> > const& a)
{
  for (size_t i = 0; i < snt.size(); ++i)
    {
      cout << format("%d:%s[") % i % V[snt[i].id()];
      for (size_t k = 0; k < a[i].size(); ++k)
	cout << (k?",":"") << a[i][k];
      cout << "] ";
    }
  cout << endl;
}


void show_pair(size_t const sid)
{
  vector<Token> s,t;
  fill_token_seq(*bg.V1,src[sid],s);
  fill_token_seq(*bg.V2,trg[sid],t);
  vector<vector<int> > a1(s.size()),a2(t.size());
  istringstream buf(aln[sid]);
  cout << aln[sid] << endl;
  int i,k; char c;
  while (buf >> i >> c >> k)
    {
      a1[i].push_back(k);
      a2[k].push_back(i);
      cout << i << "-" << k << " ";
    }
  cout << endl;
  show_snt(cout,*bg.V1,s,a1);
  show_snt(cout,*bg.V2,t,a2);
}

int main(int argc, char* argv[])
{
  if (argc < 5)
    {
      cerr << "usage: " << argv[0]
	   << " <bg base name> <L1> <L2> <fg base name>"
	   << endl;
      exit(1);
    }
  bg.open(argv[1],argv[2],argv[3]);
  SPTR<imbitext> fg(new imbitext(bg.V1,bg.V2));
  string base = argv[4];
  if (*base.rbegin() != '.') base += '.';
  string srcfile = base + argv[2];
  string trgfile = base + argv[3];
  string alnfile = base + "symal";
  read_data(srcfile,src);
  read_data(trgfile,trg);
  read_data(alnfile,aln);
  fg = fg->add(src,trg,aln);

  vector<float> bias(src.size(),1./(src.size()-1));
  for (size_t sid = 0; sid < src.size(); ++sid)
    {
      bias[sid] = 0;
      // cout << src[sid] << endl << trg[sid] << endl;
      // show_pair(sid);
      vector<Token> snt;
      fill_token_seq(*bg.V1,src[sid],snt);
      vector<vector<SPTR<vector<PhrasePair<Token> > > > > FG,BG;
      fg->lookup(snt,*fg->I1,FG,NULL,NULL,&bias,true);
      bg.lookup(snt,*bg.I1,BG,NULL,NULL,NULL,true);
      set<SPTR<vector<PhrasePair<Token> > > > seen;
      for (size_t i = 0; i < snt.size(); ++i)
	{
	  Bitext<Token>::iter m0(fg->I1.get());
	  Bitext<Token>::iter m1(bg.I1.get());
	  for (size_t k = 0; k < FG[i].size(); ++k)
	    {
	      if (!m0.extend(snt[i+k].id())) break;
	      if (k && m0.approxOccurrenceCount() < 2) break;
	      if (m1.size() == k && (!m1.extend(snt[i+k].id()) ||
				     m1.approxOccurrenceCount() < 25))
		{
		  cout << toString((*fg->V1), m0.getToken(0), m0.size()) << " "
		       << int(m0.approxOccurrenceCount());
		  if (m1.size() == k + 1)
		    cout  << " "<< int(m1.approxOccurrenceCount());
		  else if (m1.size())
		    cout  << " ["<< int(m1.approxOccurrenceCount()) << "]";
		  else
		    cout << " NEW!";
		  cout << endl;
		}
	      if (m0.approxOccurrenceCount() < 2) break;
	      BOOST_FOREACH(PhrasePair<Token> const& pp, *FG[i][k])
		{
		  if (pp.joint < 2) continue;
		  SPTR<pstats> bgstats;
		  jstats const* bgjstats = NULL;
		  Bitext<Token>::iter m2(bg.I2.get(), pp.start2, pp.len2);
		  if (m1.approxOccurrenceCount() > 5000 ||
		      m2.approxOccurrenceCount() > 5000)
		    continue;
		  if (m1.size() == pp.len1 && m2.size() == pp.len2)
		    {
		      bgstats = bg.lookup(m1,NULL);
		      if (bgstats)
			{
			  pstats::trg_map_t::const_iterator mx;
			  mx = bgstats->trg.find(m2.getPid());
			  if (mx != bgstats->trg.end())
			    bgjstats = &mx->second;
			}
		    }
		  cout << toString(*fg->V1, pp.start1, pp.len1) << " ::: "
		       << toString(*fg->V2, pp.start2, pp.len2) << " "
		       << format("[%u/%u/%u]") % pp.good1 % pp.joint % pp.good2;
		  if (bgjstats)
		    cout << " " << (format("[%u/%u/%u]")
				    % bgstats->good % bgjstats->rcnt()
				    % (bgjstats->cnt2() * bgstats->good
				       / bgstats->raw_cnt));
		  else if (m1.size() == pp.len1)
		    cout << " " << int(m1.approxOccurrenceCount());
		  cout << endl;

		}
	    }
	}
      bias[sid] = 1./(src.size()-1);
    }
  exit(0);
}



