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
#include "mm/ug_bitext_sampler.h"

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
  return path.substr(p+1, suffix == &path[k] ? k-p-1 : path.size() - p);
}

int main(int argc, char* argv[])
{
  boost::intrusive_ptr<bitext_t> B(new bitext_t);
  B->open(argv[1],argv[2],argv[3]);
  string line;
  string ifile = argv[4];
  string docname = basename(ifile, string(".") + argv[2] + ".gz");
  id_type docid = B->docname2docid(docname);
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
	  for (size_t k = i; k < snt.size() && m.extend(snt[k]); ++k);
	  for (size_t num_occurrences = m.ca(); m.size(); m.up())
	    {
	      if (size_t(m.ca()) == num_occurrences) continue;
	      num_occurrences = m.ca();
	      SPTR<SamplingBias const> zilch;
	      BitextSampler<Token> s(B, m, zilch, 1000, 1000, 
				     sapt::random_sampling);
	      s();
	      if (s.stats()->trg.size() == 0) continue;
	      // if (s.stats()->indoc[docname] > 10) continue;
	      sapt::pstats::indoc_map_t::const_iterator d
		= s.stats()->indoc.find(docid);
	      size_t indoccnt = d != s.stats()->indoc.end() ? d->second : 0;
	      cout << m.size() << " : " << m.str(B->V1.get()) << " (" 
		   << s.stats()->trg.size() << " entries; " 
		   << indoccnt << "/" << s.stats()->good 
		   << " samples in domain)" << endl;
	      vector<PhrasePair<Token> > ppairs;
	      PhrasePair<Token>::SortDescendingByJointCount sorter;
	      expand(m,*B,*s.stats(),ppairs,NULL);
	      sort(ppairs.begin(),ppairs.end(),sorter);
	      boost::format fmt("%4d/%d/%d |%s| (%4.2f : %4.2f)"); 
	      BOOST_FOREACH(PhrasePair<Token>& ppair, ppairs)
		{
		  if (ppair.joint * 100 < ppair.good1) break;
		  ppair.good2 = ppair.raw2 * float(ppair.good1)/ppair.raw1;
		  ppair.good2 = max(ppair.good2, ppair.joint);

#if 0
		  cout << "\t" 
		       << (fmt % ppair.joint % ppair.good1 % ppair.good2
			   % B->T2->pid2str(B->V2.get(),ppair.p2)
			   % (float(ppair.joint)/ppair.good1)
			   % (float(ppair.joint)/ppair.good2)
			   ) << "\n";
		  typedef std::map<uint32_t, uint32_t>::const_iterator iter;
		  for (iter d = ppair.indoc.begin(); d != ppair.indoc.end(); ++d)
		    {
		      // if (d != ppair.indoc.begin()) cout << "; ";
		      cout << (boost::format("\t\t%4d %s") % d->second 
			       % B->docid2name(d->first))  
			   << endl;
		    }
		  cout << endl;
#else
		  cout << "\t" 
		       << (fmt % ppair.joint % ppair.good1 % ppair.good2
			   % B->T2->pid2str(B->V2.get(),ppair.p2)
			   % (float(ppair.joint)/ppair.good1)
			   % (float(ppair.joint)/ppair.good2)
			   ) << " [";
		  typedef std::map<uint32_t, uint32_t>::const_iterator iter;
		  for (iter d = ppair.indoc.begin(); d != ppair.indoc.end(); ++d)
		    {
		      if (d != ppair.indoc.begin()) cout << "; ";
		      cout << (boost::format("%s: %d") % B->docid2name(d->first)
			       % d->second) ;
		    }
		  cout << "]" << endl;

#endif

		}
	    }
	}
    }
}
