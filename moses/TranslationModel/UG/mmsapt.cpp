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
    : PhraseDictionary(description,line), ofactor(1,0)
  {
    this->init(line);
  }

  Mmsapt::
  Mmsapt(string const& line)
    : PhraseDictionary("Mmsapt",line), ofactor(1,0)
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
    poolCounts = true;
  }

  void
  Mmsapt::
  Load()
  {
    btfix.open(bname, L1, L2);
    size_t num_feats;
    // TO DO: should we use different lbop parameters 
    //        for the relative-frequency based features?
    num_feats  = calc_pfwd_fix.init(0,lbop_parameter);
    num_feats  = calc_pbwd_fix.init(num_feats,lbop_parameter);
    num_feats  = calc_lex.init(num_feats, bname + L1 + "-" + L2 + ".lex");
    num_feats  = apply_pp.init(num_feats);
    if (num_feats < this->m_numScoreComponents)
      {
	poolCounts = false;
	num_feats  = calc_pfwd_dyn.init(num_feats,lbop_parameter);
	num_feats  = calc_pbwd_dyn.init(num_feats,lbop_parameter);
      }
    btdyn.reset(new imBitext<Token>(btfix.V1, btfix.V2));
    if (num_feats != this->m_numScoreComponents)
      {
	ostringstream buf;
	buf << "At " << __FILE__ << ":" << __LINE__
	    << ": number of feature values provided by Phrase table"
	    << " does not match number specified in Moses config file!";
	throw buf.str().c_str();
      }
    // cerr << "MMSAPT provides " << num_feats << " features at " 
    // << __FILE__ << ":" << __LINE__ << endl;
  }

  void
  Mmsapt::
  add(string const& s1, string const& s2, string const& a)
  {
    vector<string> S1(1,s1);
    vector<string> S2(1,s2);
    vector<string> ALN(1,a);
    boost::lock_guard<boost::mutex> guard(this->lock);
    btdyn = btdyn->add(S1,S2,ALN);
  }


  TargetPhrase* 
  Mmsapt::
  createTargetPhrase(Phrase        const& src, 
		     Bitext<Token> const& bt, 
		     PhrasePair    const& pp) const
  {
    Word w; uint32_t sid,off,len;    
    TargetPhrase* tp = new TargetPhrase();
    parse_pid(pp.p2, sid, off, len);
    Token const* x = bt.T2->sntStart(sid) + off;
    for (uint32_t k = 0; k < len; ++k)
      {
	StringPiece wrd = (*bt.V2)[x[k].id()];
	w.CreateFromString(Output,ofactor,wrd,false);
	tp->AddWord(w);
      }
    tp->GetScoreBreakdown().Assign(this, pp.fvals);
    tp->Evaluate(src);
    return tp;
  }

  // process phrase stats from a single parallel corpus
  void
  Mmsapt::
  process_pstats
  (Phrase   const& src,
   uint64_t const  pid1, 
   pstats   const& stats, 
   Bitext<Token> const & bt, 
   TargetPhraseCollection* tpcoll
   ) const
  {
    PhrasePair pp;   
    pp.init(pid1, stats, this->m_numScoreComponents);
    apply_pp(bt,pp);
    boost::unordered_map<uint64_t,jstats>::const_iterator t;
    for (t = stats.trg.begin(); t != stats.trg.end(); ++t)
      {
   	pp.update(t->first,t->second);
	calc_lex(bt,pp);
	calc_pfwd_fix(bt,pp);
	calc_pbwd_fix(bt,pp);
	tpcoll->Add(createTargetPhrase(src,bt,pp));
      }
  }

  // process phrase stats from a single parallel corpus
  bool
  Mmsapt::
  pool_pstats(Phrase   const& src,
	      uint64_t const  pid1a, 
	      pstats        * statsa, 
	      Bitext<Token> const & bta,
	      uint64_t const  pid1b, 
	      pstats   const* statsb, 
	      Bitext<Token> const & btb,
	      TargetPhraseCollection* tpcoll) const
  {
    PhrasePair pp;
    if (statsa && statsb)
      pp.init(pid1b, *statsa, *statsb, this->m_numScoreComponents);
    else if (statsa)
      pp.init(pid1b, *statsa, this->m_numScoreComponents);
    else if (statsb)
      pp.init(pid1b, *statsb, this->m_numScoreComponents);
    else return false; // throw "no stats for pooling available!";

    apply_pp(bta,pp);
    boost::unordered_map<uint64_t,jstats>::const_iterator b;
    boost::unordered_map<uint64_t,jstats>::iterator a;
    if (statsb)
      {
	for (b = statsb->trg.begin(); b != statsb->trg.end(); ++b)
	  {
	    uint32_t sid,off,len;    
	    parse_pid(b->first, sid, off, len);
	    Token const* x = bta.T2->sntStart(sid) + off;
	    TSA<Token>::tree_iterator m(bta.I2.get(),x,x+len);
	    if (m.size() == len) 
	      {
		;
		if (statsa && ((a = statsa->trg.find(m.getPid())) 
			       != statsa->trg.end()))
		  {
		    pp.update(b->first,a->second,b->second);
		    a->second.invalidate();
		  }
		else 
		  pp.update(b->first,m.approxOccurrenceCount(),
			    b->second);
	      }
	    else pp.update(b->first,b->second);
	    calc_lex(btb,pp);
	    calc_pfwd_fix(btb,pp);
	    calc_pbwd_fix(btb,pp);
	    tpcoll->Add(createTargetPhrase(src,btb,pp));
	  }
      }
    if (!statsa) return statsb != NULL;
    for (a = statsa->trg.begin(); a != statsa->trg.end(); ++a)
      {
	uint32_t sid,off,len;
	if (!a->second.valid()) continue;
	parse_pid(a->first, sid, off, len);
	if (btb.T2)
	  {
	    Token const* x = btb.T2->sntStart(sid) + off;
	    TSA<Token>::tree_iterator m(btb.I2.get(), x, x+len);
	    if (m.size() == len) 
	      pp.update(a->first,m.approxOccurrenceCount(),a->second);
	    else 
	      pp.update(a->first,a->second);
	  }
	else 
	  pp.update(a->first,a->second);
	calc_lex(bta,pp);
	calc_pfwd_fix(bta,pp);
	calc_pbwd_fix(bta,pp);
	tpcoll->Add(createTargetPhrase(src,bta,pp));
      }
    return true;
}
  
  
  // process phrase stats from a single parallel corpus
  bool
  Mmsapt::
  combine_pstats
  (Phrase   const& src,
   uint64_t const  pid1a, 
   pstats   * statsa, 
   Bitext<Token> const & bta,
   uint64_t const  pid1b, 
   pstats   const* statsb, 
   Bitext<Token> const & btb,
   TargetPhraseCollection* tpcoll
   ) const
  {
    PhrasePair ppfix,ppdyn,pool; 
    Word w;
    if (statsa) ppfix.init(pid1a,*statsa,this->m_numScoreComponents);
    if (statsb) ppdyn.init(pid1b,*statsb,this->m_numScoreComponents);
    boost::unordered_map<uint64_t,jstats>::const_iterator b;
    boost::unordered_map<uint64_t,jstats>::iterator a;
    if (statsb)
      {
	pool.init(pid1b,*statsb,0);
	apply_pp(btb,ppdyn);
	for (b = statsb->trg.begin(); b != statsb->trg.end(); ++b)
	  {
	    ppdyn.update(b->first,b->second);
	    calc_pfwd_dyn(btb,ppdyn);
	    calc_pbwd_dyn(btb,ppdyn);
	    calc_lex(btb,ppdyn);
	    
	    uint32_t sid,off,len;    
	    parse_pid(b->first, sid, off, len);
	    Token const* x = bta.T2->sntStart(sid) + off;
	    TSA<Token>::tree_iterator m(bta.I2.get(),x,x+len);
	    if (m.size() && statsa && 
		((a = statsa->trg.find(m.getPid())) 
		 != statsa->trg.end()))
	      {
		ppfix.update(a->first,a->second);
		calc_pfwd_fix(bta,ppfix,&ppdyn.fvals);
		calc_pbwd_fix(btb,ppfix,&ppdyn.fvals);
		a->second.invalidate();
	      }
	    else 
	      {
		if (m.size())
		  pool.update(b->first,m.approxOccurrenceCount(),
			      b->second);
		else
		  pool.update(b->first,b->second);
		calc_pfwd_fix(btb,pool,&ppdyn.fvals);
		calc_pbwd_fix(btb,pool,&ppdyn.fvals);
	      }
	    tpcoll->Add(createTargetPhrase(src,btb,ppdyn));
	  }
      }
    if (statsa)
      {
	pool.init(pid1a,*statsa,0);
	apply_pp(bta,ppfix);
	for (a = statsa->trg.begin(); a != statsa->trg.end(); ++a)
	  {
	    if (!a->second.valid()) continue; // done above
	    ppfix.update(a->first,a->second);
	    calc_pfwd_fix(bta,ppfix);
	    calc_pbwd_fix(bta,ppfix);
	    calc_lex(bta,ppfix);
	    
	    uint32_t sid,off,len;    
	    parse_pid(a->first, sid, off, len);
	    Token const* x = btb.T2->sntStart(sid) + off;
	    TSA<Token>::tree_iterator m(btb.I2.get(),x,x+len);
	    if (m.size())
	      pool.update(a->first,m.approxOccurrenceCount(),a->second);
	    else
	      pool.update(a->first,a->second);
	    calc_pfwd_dyn(bta,pool,&ppfix.fvals);
	    calc_pbwd_dyn(bta,pool,&ppfix.fvals);
	  }
	tpcoll->Add(createTargetPhrase(src,bta,ppfix));
      }
    return (statsa || statsb);
  }
  
  // // phrase statistics combination treating the two knowledge 
  // // sources separately with backoff to pooling when only one 
  // // of the two knowledge sources contains the phrase pair in 
  // // question
  // void
  // Mmsapt::
  // process_pstats(uint64_t const  mypid1,
  // 		 uint64_t const  otpid1,
  // 		 pstats   const& mystats,       // my phrase stats
  // 		 pstats   const* otstats,       // other phrase stats
  // 		 Bitext<Token> const & mybt,    // my bitext
  // 		 Bitext<Token> const * otbt,    // other bitext
  // 		 PhraseScorer<Token> const& mypfwd, 
  // 		 PhraseScorer<Token> const& mypbwd, 
  // 		 PhraseScorer<Token> const* otpfwd, 
  // 		 PhraseScorer<Token> const* otpbwd, 
  // 		 TargetPhraseCollection* tpcoll)
  // {
  //   boost::unordered_map<uint64_t,jstats>::const_iterator t;
  //   vector<FactorType> ofact(1,0);
  //   PhrasePair mypp,otpp,combo; 
  //   mypp.init(mypid1, mystats, this->m_numScoreComponents);
  //   if (otstats) 
  //     {
  // 	otpp.init(otpid1, *otstats, 0);
  // 	combo.init(otpid1, mystats, *otstats, 0);
  //     }
  //   else combo = mypp;
    
  //   for (t = mystats.trg.begin(); t != mystats.trg.end(); ++t)
  //     {
  // 	if (!t->second.valid()) continue; 
  // 	// we dealt with this phrase pair already; 
  // 	// see j->second.invalidate() below;
  // 	uint32_t sid,off,len; parse_pid(t->first,sid,off,len);
   
  // 	mypp.update(t->first,t->second);
  // 	apply_pp(mybt,mypp);
  // 	calc_lex (mybt,mypp);
  // 	mypfwd(mybt,mypp);
  // 	mypbwd(mybt,mypp);
	
  // 	if (otbt) // it's a dynamic phrase table
  // 	  {
  // 	    assert(otpfwd);
  // 	    assert(otpbwd);
  // 	    boost::unordered_map<uint64_t,jstats>::iterator j;
	    
  // 	    // look up the current target phrase in the other bitext
  // 	    Token const* x = mybt.T2->sntStart(sid) + off;
  // 	    TSA<TOKEN>::tree_iterator m(otbt->I2.get(),x,x+len);
  // 	    if (otstats     // source phrase exists in other bitext
  // 		&& m.size() // target phrase exists in other bitext
  // 		&& ((j = otstats->trg.find(m.getPid())) 
  // 		    != otstats->trg.end())) // phrase pair found in other bitext
  // 	      {
  // 		otpp.update(j->first,j->second);
  // 		j->second.invalidate(); // mark the phrase pair as seen
  // 		otpfwd(*otbt,otpp,&mypp.fvals);
  // 		otpbwd(*otbt,otpp,&mypp.fvals);
  // 	      }
  // 	    else 
  // 	      {
  // 		if (m.size()) // target phrase seen in other bitext, but not the phrase pair
  // 		  combo.update(t->first,m.approxOccurrenceCount(),t->second);
  // 		else
  // 		  combo.update(t->first,t->second);
  // 		(*otpfwd)(mybt,combo,&mypp.fvals);
  // 		(*otpbwd)(mybt,combo,&mypp.fvals);
  // 	      }
  // 	  }
	
  // 	// now add the phrase pair to the TargetPhraseCollection:
  // 	TargetPhrase* tp = new TargetPhrase();
  // 	for (size_t k = off; k < stop; ++k)
  // 	  {
  // 	    StringPiece wrd = (*mybt.V2)[x[k].id()];
  // 	    Word w; w.CreateFromString(Output,ofact,wrd,false);
  // 	    tp->AddWord(w);
  // 	  }
  // 	tp->GetScoreBreakdown().Assign(this,mypp.fvals);
  // 	tp->Evaluate(src);
  // 	tpcoll->Add(tp);
  //     }
  // }
  
  // This is not the most efficient way of phrase lookup! 
  TargetPhraseCollection const* 
  Mmsapt::
  GetTargetPhraseCollectionLEGACY(const Phrase& src) const
  {
    TargetPhraseCollection* ret = new TargetPhraseCollection();

    // Reserve a local copy of the dynamic bitext in its current form. /btdyn/
    // is set to a new copy of the dynamic bitext every time a sentence pair
    // is added. /dyn/ keeps the old bitext around as long as we need it.
    sptr<imBitext<Token> > dyn;
    { // braces are needed for scoping mutex lock guard!
      boost::lock_guard<boost::mutex> guard(this->lock);
      dyn = btdyn;
    }

    vector<id_type> sphrase(src.GetSize());
    for (size_t i = 0; i < src.GetSize(); ++i)
      {
	Factor const* f = src.GetFactor(i,input_factor);
	id_type wid = (*btfix.V1)[f->ToString()]; 
	sphrase[i] = wid;
      }

    TSA<Token>::tree_iterator mfix(btfix.I1.get()), mdyn(dyn->I1.get());
    for (size_t i = 0; mfix.size() == i && i < sphrase.size(); ++i)
      mfix.extend(sphrase[i]);
    
    if (dyn->I1.get())
      {
	for (size_t i = 0; mdyn.size() == i && i < sphrase.size(); ++i)
	  mdyn.extend(sphrase[i]);
      }

    sptr<pstats> sfix,sdyn;
    if (mfix.size() == sphrase.size())
      {
	// do we need this lock here? 
	// Is it used here to control the total number of running threads???
	boost::lock_guard<boost::mutex> guard(this->lock);
	sfix = btfix.lookup(mfix);
      }
    if (mdyn.size() == sphrase.size())
      sdyn = dyn->lookup(mdyn);
    if (poolCounts)
      {
	if (!pool_pstats(src, mfix.getPid(),sfix.get(),btfix, 
			 mdyn.getPid(),sdyn.get(),*dyn,ret))
	  return NULL;
      }
    else if (!combine_pstats(src, mfix.getPid(),sfix.get(),btfix, 
			     mdyn.getPid(),sdyn.get(),*dyn,ret))
      return NULL;
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
    throw "CreateRuleLookupManager is currently not supported in Mmsapt!";
  }

  template<typename Token>
  void 
  fill_token_seq(TokenIndex& V, string const& line, vector<Token>& dest)
  {
    istringstream buf(line); string w;
    while (buf>>w) dest.push_back(Token(V[w]));
  }


}
