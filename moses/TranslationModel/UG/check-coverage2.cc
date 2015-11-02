// for each word in the input, keep track of the longest matching ngram covering it
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
  bitext_t B;
  B.open(argv[1],argv[2],argv[3]);
  B.V1->setDynamic(true);
  string line;
  string ifile = argv[4];
  string docname = basename(ifile, string(".") + argv[2] + ".gz");
  boost::iostreams::filtering_istream in;
  ugdiss::open_input_stream(ifile,in);
  while(getline(in,line))
    {
      cout << line << " [" << docname << "]" << endl;
      vector<id_type> snt;
      B.V1->fillIdSeq(line,snt);
      vector<size_t> match(snt.size(),0);
      for (size_t i = 0; i < snt.size(); ++i)
	{
	  bitext_t::iter m(B.I1.get());
	  for (size_t k = i; k < snt.size() && m.extend(snt[k]); ++k);
	  for (size_t j = 0; j < m.size(); ++j) 

	    match[i+j] = max(match[i+j], m.size());
	}
      for (size_t i = 0; i < snt.size(); ++i)
	cout << setw(3) << match[i] << " " << (*B.V1)[snt[i]] << endl;
    }
}
