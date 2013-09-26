#include "mmsapt.h"
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

namespace Moses
{
  using namespace bitext;
  using namespace std;
  using namespace boost;

  void 
  parseLine(string const& line, map<string,string> & params)
  {
    char_separator<char> sep("; ");
    tokenizer<char_separator<char> > tokens(line,sep);
    BOOST_FOREACH(string const& t,tokens)
      {
	size_t i = t.find_first_not_of(" =");
	size_t j = t.find_first_of(" =",i+1);
	size_t k = t.find_first_not_of(" =",j+1);
	assert(i != string::npos);
	assert(k != string::npos);
	params[t.substr(i,j)] = t.substr(k);
      }
  }

  Mmsapt::
  Mmsapt(string const& description, string const& line)
    : PhraseDictionary(description,line)
  {
    this->init(line);
  }

  Mmsapt::
  Mmsapt(string const& line)
    : PhraseDictionary("Mmsapt",line)
  {
    this->init(line);
  }

  void
  Mmsapt::
  init(string const& line)
  {
    map<string,string> param;
    parseLine(line,param);
    bname = param["base"];
    L1    = param["L1"];
    L2    = param["L2"];
    assert(bname.size());
    assert(L1.size());
    assert(L2.size());
    map<string,string>::const_iterator m;
    m = param.find("smooth");
    lbop_parameter = m != param.end() ? atof(m->second.c_str()) : .05;
    m = param.find("max-samples");
    default_sample_size = m != param.end() ? atoi(m->second.c_str()) : 1000;
    this->m_numScoreComponents = atoi(param["num-features"].c_str());
    // num_features = 0;
    m = param.find("ifactor");
    input_factor = m != param.end() ? atoi(m->second.c_str()) : 0;
  }

  void
  Mmsapt::
  Load()
  {
    bt.open(bname, L1, L2);
    size_t num_feats;
    num_feats = calc_pfwd.init(0,lbop_parameter);
    num_feats = calc_pbwd.init(num_feats,lbop_parameter);
    num_feats = calc_lex.init(num_feats, bname + L1 + "-" + L2 + ".lex");
    num_feats = apply_pp.init(num_feats);
    assert (num_feats == this->m_numScoreComponents);
    // cerr << "MMSAPT provides " << num_feats << " features at " 
    // << __FILE__ << ":" << __LINE__ << endl;
  }

  
  // this is not the most efficient way of phrase lookup! 
  TargetPhraseCollection const* 
  Mmsapt::
  GetTargetPhraseCollectionLEGACY(const Phrase& src) const
  {
    TSA<Token>::tree_iterator m(bt.I1.get());
    for (size_t i = 0; i < src.GetSize(); ++i)
      {
	Factor const* f = src.GetFactor(i,input_factor);
	id_type wid = (*bt.V1)[f->ToString()]; 
	// cout << (*bt.V1)[wid] << " ";
	if (!m.extend(wid)) break;
      }
#if 0
    cout << endl;
    Token const* sphrase = m.getToken(0);
    for (size_t i = 0; i < m.size(); ++i)
      cout << (*bt.V1)[sphrase[i].id()] << " ";
    cout << endl;
#endif

    sptr<pstats> s;
    if (m.size() < src.GetSize()) return NULL;
    {
      boost::lock_guard<boost::mutex> guard(this->lock);
      s = bt.lookup(m);
    }
    PhrasePair pp; pp.init(m.getPid(), *s, this->m_numScoreComponents);
    TargetPhraseCollection* ret = new TargetPhraseCollection();

    vector<FactorType> ofact(1,0);
    boost::unordered_map<uint64_t,jstats>::const_iterator t;
    for (t = s->trg.begin(); t != s->trg.end(); ++t)
      {
	pp.update(t->first,t->second);
	calc_pfwd(bt,pp);
	calc_pbwd(bt,pp);
	calc_lex (bt,pp);
	apply_pp (bt,pp);

	uint32_t sid,off,len;
	parse_pid(t->first,sid,off,len);
	size_t stop = off + len;
	Token const* x = bt.T2->sntStart(sid);

	TargetPhrase* tp = new TargetPhrase();
	for (size_t k = off; k < stop; ++k)
	  {
	    StringPiece wrd = (*bt.V2)[x[k].id()];
	    Word w; w.CreateFromString(Output,ofact,wrd,false);
	    tp->AddWord(w);
	  }
	tp->GetScoreBreakdown().Assign(this,pp.fvals);
	tp->Evaluate(src);
	ret->Add(tp);
      }
    ret->NthElement(m_tableLimit);
#if 0
    sort(ret->begin(), ret->end(), CompareTargetPhrase());
    cout << "SOURCE PHRASE: " << src << endl;
    size_t i = 0;
    for (TargetPhraseCollection::iterator r = ret->begin(); r != ret->end(); ++r)
      {
	cout << ++i << " " << **r << endl;
      }
#endif
    return ret;
  }

  ChartRuleLookupManager*
  Mmsapt::
  CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase &)
  {
    throw "CreateRuleLookupManager is currently not supported in Moses!";
  }

}
