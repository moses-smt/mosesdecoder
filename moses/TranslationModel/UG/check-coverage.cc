// #include "mmsapt.h"
// #include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
// #include "moses/TranslationTask.h"
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

using namespace Moses;
using namespace sapt;
using namespace std;
using namespace boost;

typedef sapt::L2R_Token<sapt::SimpleWordId> Token;
typedef mmBitext<Token> bitext_t;

struct mycmp 
{
  bool operator() (pair<string,uint32_t> const& a, 
		   pair<string,uint32_t> const& b) const
  {
    return a.second > b.second;
  }
};

string 
basename(string const path, string const suffix)
{
  size_t p = path.find_last_of("/");
  size_t k = path.size() - suffix.size();
  cout << path << " " << suffix << endl;
  cout << path.substr(0,p) << " " << path.substr(k) << endl;
  return path.substr(p, suffix == &path[k] ? k-p : path.size() - p);
}

int main(int argc, char* argv[])
{
  boost::shared_ptr<bitext_t> B(new bitext_t);
  B->open(argv[1],argv[2],argv[3]);
  string line;
  string ifile = argv[4];
  string docname = basename(ifile, string(".") + argv[2] + ".gz");
  boost::iostreams::filtering_istream in;
  ugdiss::open_input_stream(ifile,in);
  while(getline(in,line))
    {
      cout << line << " [" << docname << "]" << endl;
      vector<id_type> snt;
      B->V1->fillIdSeq(line,snt);
      for (size_t i = 0; i < snt.size(); ++i)
	{
	  bitext_t::iter m(B->I1.get());
	  for (size_t k = i; k < snt.size() && m.extend(snt[k]); ++k)
	    {
	      if (m.ca() > 500) continue;
	      sapt::tsa::ArrayEntry I(m.lower_bound(-1));
	      char const* stop = m.upper_bound(-1);
	      map<string,uint32_t> cnt;
	      while (I.next != stop)
		{
		  m.root->readEntry(I.next,I);
		  ++cnt[B->sid2docname(I.sid)];
		}
	      cout << setw(8) << int(m.ca()) << " " 
		   << B->V1->toString(&snt[i],&snt[k+1]) << endl;
	      typedef pair<string,uint32_t> entry;
	      vector<entry> ranked; ranked.reserve(cnt.size());
	      BOOST_FOREACH(entry const& e, cnt) ranked.push_back(e);
	      sort(ranked.begin(),ranked.end(),mycmp());
	      BOOST_FOREACH(entry const& e, ranked)
		cout << setw(12) << " " << e.second << " " << e.first << endl; 
	      cout << endl;
	    }
	}
    }
}
