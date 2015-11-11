#if 0
// temporarily disabled; needs to be adapted to changes in the API
#include "mmsapt.h"
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "moses/TranslationModel/UG/generic/program_options/ug_splice_arglist.h"
#include <boost/algorithm/string/predicate.hpp>
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
using namespace boost::algorithm;

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
    // return cmp(*my_tpc[a], *my_tpc[b]);
    return (my_tpc[a]->GetScoreBreakdown().GetWeightedScore() >
	    my_tpc[b]->GetScoreBreakdown().GetWeightedScore());
  }
};

int main(int argc, char* argv[])
{

  string vlevel = "alt"; // verbosity level
  vector<pair<string,int> > argfilter(5);
  argfilter[0] = std::make_pair(string("--spe-src"),1);
  argfilter[1] = std::make_pair(string("--spe-trg"),1);
  argfilter[2] = std::make_pair(string("--spe-aln"),1);
  argfilter[3] = std::make_pair(string("--spe-show"),1);

  char** my_args; int my_acnt;
  char** mo_args; int mo_acnt;
  filter_arguments(argc, argv, mo_acnt, &mo_args, my_acnt, &my_args, argfilter);

  ifstream spe_src,spe_trg,spe_aln;
  // instead of translating show coverage by phrase tables
  for (int i = 0; i < my_acnt; i += 2)
    {
      if (!strcmp(my_args[i],"--spe-src"))
	spe_src.open(my_args[i+1]);
      else if (!strcmp(my_args[i],"--spe-trg"))
	spe_trg.open(my_args[i+1]);
      else if (!strcmp(my_args[i],"--spe-aln"))
	spe_aln.open(my_args[i+1]);
      else if (!strcmp(my_args[i],"--spe-show"))
	vlevel = my_args[i+1];
    }

  Parameter params;
  if (!params.LoadParam(mo_acnt,mo_args) ||
      !StaticData::LoadDataStatic(&params, mo_args[0]))
    exit(1);

  StaticData const& global = StaticData::Instance();
  global.SetVerboseLevel(0);
  vector<FactorType> ifo = global.GetInputFactorOrder();

  PhraseDictionary* PT = PhraseDictionary::GetColl()[0];
  Mmsapt* mmsapt = dynamic_cast<Mmsapt*>(PT);
  if (!mmsapt)
    {
      cerr << "Phrase table implementation not supported by this utility." << endl;
      exit(1);
    }
  mmsapt->SetTableLimit(0);

  string srcline,trgline,alnline;
  cout.precision(2);
  vector<string> fname = mmsapt->GetFeatureNames();
  while (getline(spe_src,srcline))
    {
      UTIL_THROW_IF2(!getline(spe_trg,trgline), HERE
		     << ": missing data for online updates.");
      UTIL_THROW_IF2(!getline(spe_aln,alnline), HERE
		     << ": missing data for online updates.");
      cout << string(80,'-') << "\n" << srcline << "\n" << trgline << "\n" << endl;

      // cout << srcline << " " << HERE << endl;
      Sentence snt;
      istringstream buf(srcline+"\n");
      if (!snt.Read(buf,ifo)) break;
      // cout << Phrase(snt) << endl;
      int dynprovidx = -1;
      for (size_t i = 0; i < fname.size(); ++i)
	{
	  if (starts_with(fname[i], "prov-1."))
	    dynprovidx = i;
	}
      cout << endl;
      for (size_t i = 0; i < snt.GetSize(); ++i)
	{
	  for (size_t k = i; k < snt.GetSize(); ++k)
	    {
	      Phrase p = snt.GetSubString(Range(i,k));
	      if (!mmsapt->PrefixExists(p)) break;
	      TargetPhraseCollection const* trg = PT->GetTargetPhraseCollectionLEGACY(p);
	      if (!trg || !trg->GetSize()) continue;

	      bool header_done = false;
	      bool has_dynamic_match = vlevel == "all" || vlevel == "ALL";
	      vector<size_t> order; order.reserve(trg->GetSize());
	      size_t stop = trg->GetSize();

	      vector<size_t> o2(trg->GetSize());
	      for (size_t i = 0; i < stop; ++i) o2[i] = i;
	      sort(o2.begin(),o2.end(),TargetPhraseIndexSorter(*trg));

	      for (size_t r = 0; r < stop; ++r) // r for rank
		{
		  if (vlevel != "ALL")
		    {
		      Phrase const& phr = static_cast<Phrase const&>(*(*trg)[o2[r]]);
		      ostringstream buf; buf << phr;
		      string tphrase = buf.str();
		      tphrase.erase(tphrase.size()-1);
		      size_t s = trgline.find(tphrase);
		      if (s == string::npos) continue;
		      size_t e = s + tphrase.size();
		      if ((s && trgline[s-1] != ' ') || (e < trgline.size() && trgline[e] != ' '))
			continue;
		    }
		  order.push_back(r);
		  if (!has_dynamic_match)
		    {
		      ScoreComponentCollection const& scc = (*trg)[o2[r]]->GetScoreBreakdown();
		      ScoreComponentCollection::IndexPair idx = scc.GetIndexes(PT);
		      FVector const& scores = scc.GetScoresVector();
		      has_dynamic_match = scores[idx.first + dynprovidx] > 0;
		    }
		}
	      if ((vlevel == "alt" || vlevel == "new") && !has_dynamic_match)
		continue;


	      BOOST_FOREACH(size_t const& r, order)
		{
		  ScoreComponentCollection const& scc = (*trg)[o2[r]]->GetScoreBreakdown();
		  ScoreComponentCollection::IndexPair idx = scc.GetIndexes(PT);
		  FVector const& scores = scc.GetScoresVector();
		  float wscore = scc.GetWeightedScore();
		  if (vlevel == "new" && scores[idx.first + dynprovidx] == 0)
		    continue;
		  if (!header_done)
		    {
		      cout << endl;
		      if (trg->GetSize() == 1)
			cout << p << " (1 translation option)" << endl;
		      else
			cout << p << " (" << trg->GetSize() << " translation options)" << endl;
		      header_done = true;
		    }
		  Phrase const& phr = static_cast<Phrase const&>(*(*trg)[o2[r]]);
		  cout << setw(3) << r+1 << " " << phr << endl;
		  cout << "   ";
		  BOOST_FOREACH(string const& fn, fname)
		    cout << " " << format("%10.10s") % fn;
		  cout << endl;
		  cout << "   ";
		  for (size_t x = idx.first; x < idx.second; ++x)
		    {
		      size_t j = x-idx.first;
		      float f = (mmsapt && mmsapt->isLogVal(j)) ? exp(scores[x]) : scores[x];
		      string fmt = (mmsapt && mmsapt->isInteger(j)) ? "%10d" : "%10.8f";
		      if (starts_with(fname[j], "lex")) fmt = "%10.3e";
		      else if (starts_with(fname[j], "prov-1."))
			{
			  f = round(f/(1-f));
			  fmt = "%10d";
			}
		      cout << " " << format(fmt) % (mmsapt->isInteger(j) ? round(f) : f);
		    }
		  cout << " " << format("%10.3e") % exp(wscore)
		       << " " << format("%10.3e") % exp((*trg)[o2[r]]->GetFutureScore()) << endl;
		}
	      mmsapt->Release(trg);
	      continue;
	    }
	}
      mmsapt->add(srcline,trgline,alnline);
    }
  // }
  exit(0);
}
#endif


