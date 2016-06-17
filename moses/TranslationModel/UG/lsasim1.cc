#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include "mm/tpt_tokenindex.h"
#include "generic/sorting/NBestList.h"
#include "generic/sorting/VectorIndexSorter.h"
#include <algorithm>

using namespace std;
using namespace sapt;
using namespace Moses;
namespace po=boost::program_options;

TokenIndex V1, V2;

void interpret_args(int ac, char* av[]);

string bname, L1, L2;
uint32_t num_cols;
float const* T1, *T2;

string lookup;

float 
cosine_sim(uint32_t const a, uint32_t const b)
{
  float const* x = T1 + (a * num_cols);
  float const* y = T2 + (b * num_cols);
  float ret = 0, xx = 0, yy = 0;
  for (size_t i = num_cols - 1; i-- > 0;)
    {
      ret += x[i] * y[i];
      xx  += x[i] * x[i];
      yy  += y[i] * y[i];
    }
  return ret / (sqrt(xx) * sqrt(yy));
}


void 
show(vector<uint32_t> const& snt, 
     vector<vector<pair<float,uint32_t> > > const& cands,
     TokenIndex const& V1, TokenIndex const& V2)
{
  greater<pair<float,uint32_t> > better;
  for (size_t i = 0; i < snt.size(); ++i)
    {
      VectorIndexSorter<pair<float, uint32_t> > sorter(cands[i]);
      vector<size_t> order; sorter.GetOrder(order);
      cout << V1[snt[i]];
      BOOST_FOREACH(size_t k, order)
	cout << " " << cands[i][k].first << ":" << V2[cands[i][k].second];
      cout << endl;
    }
  cout << endl;
}

int 
main(int argc, char* argv[])
{
  interpret_args(argc,argv);
  V1.open(bname+L1+".tdx");
  V2.open(bname+L2+".tdx");
  boost::iostreams::mapped_file_source term1, term2;
  term1.open(bname+L1+".term-vectors");
  term2.open(bname+L2+".term-vectors");
  num_cols = *reinterpret_cast<uint32_t const*> (term1.data() + 8);
  
  T1 = reinterpret_cast<float const*>(term1.data() + 12);
  T2 = reinterpret_cast<float const*>(term2.data() + 12);
  

  string line1,line2, w;

  while (getline(cin,line1) && getline(cin,line2))
    {
      vector<uint32_t> snt1,snt2;
      V1.fillIdSeq(line1,snt1);
      V2.fillIdSeq(line2,snt2);
      
      vector<vector<pair<float,uint32_t> > > foo1(snt1.size()),foo2(snt2.size());
      
      vector<vector<size_t> > order1(snt1.size()),order2(snt2.size());

      for (size_t i = 0; i < snt1.size(); ++i)
	{
	  for (size_t k = 0; k < snt2.size(); ++k)
	    {
	      float s = cosine_sim(snt1[i],snt2[k]);
	      foo1[i].push_back(pair<float,uint32_t>(s,snt2[k]));
	      foo2[k].push_back(pair<float,uint32_t>(s,snt1[i]));
	    }
	  
	}
      for (size_t i = 0; i < snt1.size(); ++i)
	VectorIndexSorter<pair<float, uint32_t> >(foo1[i]).GetOrder(order1[i]);
      for (size_t k = 0; k < snt2.size(); ++k)
	VectorIndexSorter<pair<float, uint32_t> >(foo2[k]).GetOrder(order2[k]);
      
      for (size_t i = 0; i < snt1.size(); ++i)
	{
	  if (order2[order1[i][0]][0] == i)
	    cout << V1[snt1[i]] << " " << V2[snt2[order1[i][0]]] << endl;
	  else 
	    cout << V1[snt1[i]] << endl;
	}
      cout << endl;
      // show(snt1,foo1,V1,V2);
      // show(snt2,foo2,V2,V1);
    }
  
  // uint32_t id1 = V1[lookup];
  // NBestList<pair<float,uint32_t>, greater<pair<float,uint32_t> > > nbest(20,better);
  // for (uint32_t id2 = 0; id2 < V2.ksize(); ++id2)
  //   {
  //     // if (id2 && (id2%1000 == 0)) cerr << ".";
  //     nbest.add(pair<float,uint32_t>(cosine_sim(id1, id2), id2));
  //   }

  // cerr << endl;
  // for (size_t i = 0; i < nbest.size(); ++i)
  //   cout << nbest[i].first << " " << V2[nbest[i].second] << endl;

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
    ("lookup", po::value<string>(&lookup), "lookup term");

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
		<< " [options] <model file stem> <L1> <L2> <matrix U> <matrix S>" << std::endl;
      std::cout << o << std::endl;
      exit(0);
    }
}

