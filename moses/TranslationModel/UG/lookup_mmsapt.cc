#include "mmsapt.h"
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <iostream>

using namespace Moses;
using namespace bitext;
using namespace std;
using namespace boost;

vector<FactorType> fo(1,FactorType(0));

class SimplePhrase : public Moses::Phrase
{
  vector<FactorType> const m_fo; // factor order
public:
  SimplePhrase(): m_fo(1,FactorType(0)) {}
  
  void init(string const& s) 
  {
    istringstream buf(s); string w;
    while (buf >> w) 
      {
	Word wrd; 
	this->AddWord().CreateFromString(Input,m_fo,StringPiece(w),false,false);
      }
  }
};

class TargetPhraseIndexSorter
{
  TargetPhraseCollection const& my_tpc;
  CompareTargetPhrase cmp;
public:
  TargetPhraseIndexSorter(TargetPhraseCollection const& tpc) : my_tpc(tpc) {}
  bool operator()(size_t a, size_t b) const
  {
    return cmp(*my_tpc[a], *my_tpc[b]);
  }
};

int main(int argc, char* argv[])
{
  Parameter params;
  if (!params.LoadParam(argc,argv) || !StaticData::LoadDataStatic(&params, argv[0]))
    exit(1);

  Mmsapt* PT;
  BOOST_FOREACH(PhraseDictionary* pd, PhraseDictionary::GetColl())
    if ((PT = dynamic_cast<Mmsapt*>(pd))) break;

  string line;
  while (getline(cin,line))
    {
      SimplePhrase p; p.init(line); 
      cout << p << endl;
      TargetPhraseCollection const* trg = PT->GetTargetPhraseCollectionLEGACY(p);
      if (!trg) continue;
      vector<size_t> order(trg->GetSize()); 
      for (size_t i = 0; i < order.size(); ++i) order[i] = i;
      sort(order.begin(),order.end(),TargetPhraseIndexSorter(*trg));
      size_t k = 0;
      BOOST_FOREACH(size_t i, order)
	{
	  Phrase const& phr = static_cast<Phrase const&>(*(*trg)[i]);
	  cout << setw(3) << ++k << " " << phr << endl;
	}
      PT->Release(trg);
    }
  exit(0);
}
  
  

