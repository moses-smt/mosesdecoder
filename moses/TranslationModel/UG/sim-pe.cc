#include "mmsapt.h"
#include "moses/Manager.h"
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
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

vector<FactorType> fo(1,FactorType(0));

ostream&
operator<<(ostream& out, Hypothesis const* x)
{
  vector<const Hypothesis*> H;
  for (const Hypothesis* h = x; h; h = h->GetPrevHypo())
    H.push_back(h);
  for (; H.size(); H.pop_back())
    {
      Phrase const& p = H.back()->GetCurrTargetPhrase();
      for (size_t pos = 0 ; pos < p.GetSize() ; pos++)
	out << *p.GetFactor(pos, 0) << (H.size() ? " " : "");
    }
  return out;
}

vector<FactorType> ifo;
size_t lineNumber;

string
translate(string const& source)
{
  StaticData const& global = StaticData::Instance();

  Sentence sentence;
  istringstream ibuf(source+"\n");
  sentence.Read(ibuf,ifo);

  // Manager manager(lineNumber, sentence, global.GetSearchAlgorithm());
  Manager manager(sentence, global.GetSearchAlgorithm());
  manager.ProcessSentence();

  ostringstream obuf;
  const Hypothesis* h = manager.GetBestHypothesis();
  obuf << h;
  return obuf.str();

}

int main(int argc, char* argv[])
{
  Parameter params;
  if (!params.LoadParam(argc,argv) || !StaticData::LoadDataStatic(&params, argv[0]))
    exit(1);

  StaticData const& global = StaticData::Instance();
  global.SetVerboseLevel(0);
  ifo = global.GetInputFactorOrder();

  lineNumber = 0; // TODO: Include sentence request number here?
  string source, target, alignment;
  while (getline(cin,source))
    {
      getline(cin,target);
      getline(cin,alignment);
      cout << "[S] " << source << endl;
      cout << "[H] " << translate(source) << endl;
      cout << "[T] " << target << endl;
      Mmsapt* pdsa = reinterpret_cast<Mmsapt*>(PhraseDictionary::GetColl()[0]);
      pdsa->add(source,target,alignment);
      cout << "[X] " << translate(source) << endl;
      cout << endl;
    }
  exit(0);
}



