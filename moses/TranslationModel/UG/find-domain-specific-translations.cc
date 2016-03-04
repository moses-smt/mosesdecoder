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

#include <boost/program_options.hpp>
#include <boost/math/distributions/binomial.hpp>

namespace po=boost::program_options;
using namespace Moses;
using namespace sapt;
using namespace std;
using namespace boost;

typedef sapt::L2R_Token<sapt::SimpleWordId> Token;
typedef mmBitext<Token> bitext_t;

size_t topN;
string docname;
string reference_file;
string domain_name;
string bname, L1, L2, lookup;
string ifile;

struct mycmp 
{
  bool operator() (pair<string,uint32_t> const& a, 
		   pair<string,uint32_t> const& b) const
  {
    return a.second > b.second;
  }
};



void interpret_args(int ac, char* av[]);

string 
basename(string const path)
{
  size_t p = path.find_last_of("/");
  string dot = ".";
  size_t k = path.find((dot + L1),p+1);
  if (k == string::npos) k = path.find(dot + L1 + ".gz");
  if (k == string::npos) return path.substr(p+1);
  return path.substr(p+1, k-p-1);
}

void 
print_evidence_list(bitext_t const& B, 
		    std::map<uint32_t, uint32_t> const& indoc)
{
  typedef std::map<uint32_t, uint32_t>::const_iterator iter;
  typedef pair<size_t,string> item;
  vector<item> where; 
  where.reserve(indoc.size());
  
  for (iter d = indoc.begin(); d != indoc.end(); ++d)
    where.push_back(item(d->second, B.docid2name(d->first)));
  sort(where.begin(),where.end(),greater<item>());
  BOOST_FOREACH(item const& doc, where)
    if (domain_name == doc.second)
      cout << (boost::format("\t\t%4d ! %s") % doc.first % doc.second) << endl;
    else
      cout << (boost::format("\t\t%4d   %s") % doc.first % doc.second) << endl;
}

typedef boost::math::binomial_distribution<> binomial;

boost::shared_ptr<bitext_t> B(new bitext_t);

void
process(bitext_t::iter& m)
{
  SPTR<SamplingBias const> nobias;
  BitextSampler<Token> s(B, m, nobias, 10000, 10000, sapt::random_sampling);
  s();
  vector<PhrasePair<Token> > ppairs;
  expand(m,*B,*s.stats(),ppairs,NULL);
  PhrasePair<Token>::SortDescendingByJointCount sorter;
  sort(ppairs.begin(),ppairs.end(),sorter);
  map<uint32_t,vector<PhrasePair<Token> const*> > dspecific;
  bool printed = false;
  BOOST_FOREACH(PhrasePair<Token>& ppair, ppairs)
    {
      float p = float(ppair.joint)/ppair.good1;
      typedef pair<uint32_t,uint32_t> item;
      BOOST_FOREACH(item const& e, ppair.indoc)
	{
	  size_t trials  = s.stats()->indoc[e.first];
	  size_t succ    = e.second;
	  float expected = p * trials;
	  if (round(expected) == succ || succ < 3) continue;
	  float p0 = (expected < succ 
		      ? cdf(complement(binomial(trials, p), succ - 1))
		      : cdf(binomial(trials, p), succ));
	  if (p0 > .05) continue;
	  if (!printed) 
	    {
	      cout << m.getToken(-1)->id() 
		   << ". " << m.str(B->V1.get()) 
		   << " (ca. " << round(m.ca()) 
		   << " source phrase occurrences)" << endl;
	      printed = true;
	    }
	  cout << setw(8) << trials << boost::format("%8.1f") % expected
	       << setw(8) << succ << "   " << (succ < expected ? "- " : "+ ")
	       << (boost::format("%-30s") 
		   % B->T2->pid2str(B->V2.get(),ppair.p2)) << " ("
	       << B->docid2name(e.first) << "; p0 = " << p0 << "; t = "
	       << p << ")" << endl;
	}
    }
  if (printed) cout << endl;
}

void process(string const& line)
{
  bitext_t::iter m(B->I1.get());
  istringstream buf(line); string w; size_t ctr = 0;
  while (buf >> w) 
    {
      ++ctr;
      if (!m.extend((*B->V1)[w])) break;
    }
  if (ctr == m.size()) process(m);
  else cout << line << ": NOT FOUND" << endl;
}


int main(int argc, char* argv[])
{
  interpret_args(argc,argv);
  B->open(bname, L1, L2);
  if (lookup == "")
    {
      bitext_t::iter m(B->I1.get());
      m.extend(2);  
      do { process(m);  } while (m.over());
    }
  else if (lookup == "-")
    {
      string line;
      while (getline(cin,line)) process(line);
    }
  else process(lookup);
}

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",  "print this message")
    ;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name of corpus")
    ("L1", po::value<string>(&L1), "L1 tag")
    ("L2", po::value<string>(&L2), "L2 tag")
    ("lookup", po::value<string>(&lookup), "lookup")
    ;

  h.add(o);
  po::positional_options_description a;
  a.add("bname",1);
  a.add("L1",1);
  a.add("L2",1);
  a.add("lookup",1);

  po::store(po::command_line_parser(ac,av)
            .options(h)
            .positional(a)
            .run(),vm);
  po::notify(vm);
  if (vm.count("help"))
    {
      std::cout << "\nusage:\n\t" << av[0]
           << " [options] <model file stem> <L1> <L2>" << std::endl;
      std::cout << o << std::endl;
      exit(0);
    }
}
