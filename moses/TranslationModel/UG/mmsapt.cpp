#include "mmsapt.h"
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <algorithm>

namespace Moses
{
  using namespace bitext;
  using namespace std;
  using namespace boost;
  
  void 
  fillIdSeq(Phrase const& mophrase, size_t const ifactor,
	    TokenIndex const& V, vector<id_type>& dest)
  {
    dest.resize(mophrase.GetSize());
    for (size_t i = 0; i < mophrase.GetSize(); ++i)
      {
	Factor const* f = mophrase.GetFactor(i,ifactor);
	dest[i] = V[f->ToString()];
      }
  }
    

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

#if 0
  Mmsapt::
  Mmsapt(string const& description, string const& line)
    : PhraseDictionary(description,line), ofactor(1,0)
  {
    this->init(line);
  }
#endif

  Mmsapt::
  Mmsapt(string const& line)
    // : PhraseDictionary("Mmsapt",line), ofactor(1,0)
    : PhraseDictionary(line), ofactor(1,0), m_tpc_ctr(0)
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

    m = param.find("pfwd_denom");
    m_pfwd_denom = m != param.end() ? m->second[0] : 's';

    m = param.find("smooth");
    m_lbop_parameter = m != param.end() ? atof(m->second.c_str()) : .05;

    m = param.find("max-samples");
    m_default_sample_size = m != param.end() ? atoi(m->second.c_str()) : 1000;

    m = param.find("workers");
    m_workers = m != param.end() ? atoi(m->second.c_str()) : 8;
    m_workers = min(m_workers,24UL);

    m = param.find("cache-size");
    m_history.reserve(m != param.end() 
		      ? max(1000,atoi(m->second.c_str()))
		      : 10000);
    
    this->m_numScoreComponents = atoi(param["num-features"].c_str());
    // num_features = 0;
    m = param.find("ifactor");
    input_factor = m != param.end() ? atoi(m->second.c_str()) : 0;
    poolCounts = true;
    m = param.find("extra");
    if (m != param.end()) 
      {
	extra_data = m->second;
	// cerr << "have extra data" << endl;
      }
    // keeps track of the most frequently used target phrase collections
    // (to keep them cached even when not actively in use)
  }

  void
  Mmsapt::
  load_extra_data(string bname)
  {
    // TO DO: ADD CHECKS FOR ROBUSTNESS
    // - file existence?
    // - same number of lines?
    // - sane word alignment?
    vector<string> text1,text2,symal;
    string line;
    filtering_istream in1,in2,ina; 

    open_input_stream(bname+L1+".txt.gz",in1);
    open_input_stream(bname+L2+".txt.gz",in2);
    open_input_stream(bname+L1+"-"+L2+".symal.gz",ina);

    while(getline(in1,line)) text1.push_back(line);
    while(getline(in2,line)) text2.push_back(line);
    while(getline(ina,line)) symal.push_back(line);

    lock_guard<mutex> guard(this->lock);
    btdyn = btdyn->add(text1,text2,symal);
    assert(btdyn);
    // cerr << "Loaded " << btdyn->T1->size() << " sentence pairs" << endl;
  }
  
  void
  Mmsapt::
  Load()
  {
    btfix.num_workers = this->m_workers;
    btfix.open(bname, L1, L2);
    btfix.setDefaultSampleSize(m_default_sample_size);
    
    size_t num_feats;
    // TO DO: should we use different lbop parameters 
    //        for the relative-frequency based features?
    num_feats  = calc_pfwd_fix.init(0,m_lbop_parameter);
    num_feats  = calc_pbwd_fix.init(num_feats,m_lbop_parameter);
    num_feats  = calc_lex.init(num_feats, bname + L1 + "-" + L2 + ".lex");
    num_feats  = apply_pp.init(num_feats);
    if (num_feats < this->m_numScoreComponents)
      {
	poolCounts = false;
	num_feats  = calc_pfwd_dyn.init(num_feats,m_lbop_parameter);
	num_feats  = calc_pbwd_dyn.init(num_feats,m_lbop_parameter);
      }

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

    btdyn.reset(new imBitext<Token>(btfix.V1, btfix.V2,m_default_sample_size));
    btdyn->num_workers = this->m_workers;
    if (extra_data.size()) load_extra_data(extra_data);

    // currently not used
    LexicalPhraseScorer2<Token>::table_t & COOC = calc_lex.scorer.COOC;
    typedef LexicalPhraseScorer2<Token>::table_t::Cell cell_t;
    wlex21.resize(COOC.numCols);
    for (size_t r = 0; r < COOC.numRows; ++r)
      for (cell_t const* c = COOC[r].start; c < COOC[r].stop; ++c)
	wlex21[c->id].push_back(r);
    COOCraw.open(bname + L1 + "-" + L2 + ".coc");

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
	// cerr << (*bt.V2)[x[k].id()] << " at " << __FILE__ << ":" << __LINE__ << endl;
	StringPiece wrd = (*bt.V2)[x[k].id()];
	// if ((off+len) > bt.T2->sntLen(sid))
	// cerr << off << ";" << len << " " << bt.T2->sntLen(sid) << endl;
	assert(off+len <= bt.T2->sntLen(sid));
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
    pstats::trg_map_t::const_iterator t;
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
      pp.init(pid1a, *statsa, this->m_numScoreComponents);
    else if (statsb)
      pp.init(pid1b, *statsb, this->m_numScoreComponents);
    else return false; // throw "no stats for pooling available!";

    apply_pp(bta,pp);
    pstats::trg_map_t::const_iterator b;
    pstats::trg_map_t::iterator a;
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
	    Token const* x = bta.T2->sntStart(sid) + off;
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
    pstats::trg_map_t::const_iterator b;
    pstats::trg_map_t::iterator a;
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
	    
	    if (btb.I2)
	      {
		uint32_t sid,off,len;    
		parse_pid(a->first, sid, off, len);
		Token const* x = bta.T2->sntStart(sid) + off;
		TSA<Token>::tree_iterator m(btb.I2.get(),x,x+len);
		if (m.size())
		  pool.update(a->first,m.approxOccurrenceCount(),a->second);
		else
		  pool.update(a->first,a->second);
	      }
	    else pool.update(a->first,a->second);
	    calc_pfwd_dyn(bta,pool,&ppfix.fvals);
	    calc_pbwd_dyn(bta,pool,&ppfix.fvals);
	  }
	if (ppfix.p2)
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
  
  Mmsapt::
  TargetPhraseCollectionWrapper::
  TargetPhraseCollectionWrapper(size_t r, uint64_t k)
    : revision(r), key(k), refCount(0), idx(-1)
  { }

  Mmsapt::
  TargetPhraseCollectionWrapper::
  ~TargetPhraseCollectionWrapper()
  {
    assert(this->refCount == 0);
  }

  

  // This is not the most efficient way of phrase lookup! 
  TargetPhraseCollection const* 
  Mmsapt::
  GetTargetPhraseCollectionLEGACY(const Phrase& src) const
  {
    // map from Moses Phrase to internal id sequence
    vector<id_type> sphrase; 
    fillIdSeq(src,input_factor,*btfix.V1,sphrase);
    if (sphrase.size() == 0) return NULL;
    
    // lookup in static bitext 
    TSA<Token>::tree_iterator mfix(btfix.I1.get(),&sphrase[0],sphrase.size());

    // lookup in dynamic bitext
    // Reserve a local copy of the dynamic bitext in its current form. /btdyn/
    // is set to a new copy of the dynamic bitext every time a sentence pair
    // is added. /dyn/ keeps the old bitext around as long as we need it.
    sptr<imBitext<Token> > dyn;
    { // braces are needed for scoping mutex lock guard!
      boost::lock_guard<boost::mutex> guard(this->lock);
      dyn = btdyn;
    }
    assert(dyn);
    TSA<Token>::tree_iterator mdyn(dyn->I1.get());
    if (dyn->I1.get())
      {
	for (size_t i = 0; mdyn.size() == i && i < sphrase.size(); ++i)
	  mdyn.extend(sphrase[i]);
      }

#if 0
    cerr << src << endl;
    cerr << mfix.size() << ":" << mfix.getPid() << " "
	 << mdyn.size() << " " << mdyn.getPid() << endl;
#endif

    // phrase not found in either
    if (mdyn.size() != sphrase.size() && 
	mfix.size() != sphrase.size()) 
      return NULL; // not found

    // cache lookup:

    uint64_t phrasekey;
    if (mfix.size() == sphrase.size())
      phrasekey = (mfix.getPid()<<1);
    else
      phrasekey = (mdyn.getPid()<<1)+1;

    size_t revision = dyn->revision();
    {
      boost::lock_guard<boost::mutex> guard(this->lock);
      tpc_cache_t::iterator c = m_cache.find(phrasekey);
      if (c != m_cache.end() && c->second->revision == revision)
	return encache(c->second);
    }
    
    // not found or not up to date
    sptr<pstats> sfix,sdyn;
    if (mfix.size() == sphrase.size())
      sfix = btfix.lookup(mfix);
    if (mdyn.size() == sphrase.size())
      sdyn = dyn->lookup(mdyn);
    
    TargetPhraseCollectionWrapper* 
      ret = new TargetPhraseCollectionWrapper(revision,phrasekey);
    if ((poolCounts && 
	 pool_pstats(src, mfix.getPid(),sfix.get(),btfix, 
		     mdyn.getPid(),sdyn.get(),*dyn,ret))
	|| combine_pstats(src, mfix.getPid(),sfix.get(),btfix, 
			  mdyn.getPid(),sdyn.get(),*dyn,ret))
      {
	ret->NthElement(m_tableLimit);
#if 0
	sort(ret->begin(), ret->end(), CompareTargetPhrase());
	cout << "SOURCE PHRASE: " << src << endl;
	size_t i = 0;
	for (TargetPhraseCollection::iterator r = ret->begin(); r != ret->end(); ++r)
	  {
	    cout << ++i << " " << **r << endl;
	    FVector fv = (*r)->GetScoreBreakdown().CreateFVector();
	    typedef pair<Moses::FName,float> item_t;
	    BOOST_FOREACH(item_t f, fv)
	      cout << f.first << ":" << f.second << " ";
	    cout << endl;
	  }
#endif
      }
    boost::lock_guard<boost::mutex> guard(this->lock);
    m_cache[phrasekey] = ret;
    return encache(ret);
  }

  void
  Mmsapt::
  CleanUpAfterSentenceProcessing(const InputType& source)
  { }


  ChartRuleLookupManager*
  Mmsapt::
  CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase &)
  {
    throw "CreateRuleLookupManager is currently not supported in Mmsapt!";
  }

  ChartRuleLookupManager*
  Mmsapt::
  CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase &,
			  size_t UnclearWhatThisVariableIsSupposedToAccomplishBecauseNobodyBotheredToDocumentItInPhraseTableDotHButIllTakeThisAsAnOpportunityToComplyWithTheMosesConventionOfRidiculouslyLongVariableAndClassNames)
  {
    throw "CreateRuleLookupManager is currently not supported in Mmsapt!";
  }

  void 
  Mmsapt::
  InitializeForInput(InputType const& source)
  {
    // assert(0);
  }

  bool operator<(timespec const& a, timespec const& b)
  {
    if (a.tv_sec != b.tv_sec) return a.tv_sec < b.tv_sec;
    return (a.tv_nsec < b.tv_nsec);
  }

  bool operator>=(timespec const& a, timespec const& b)
  {
    if (a.tv_sec != b.tv_sec) return a.tv_sec > b.tv_sec;
    return (a.tv_nsec >= b.tv_nsec);
  }

  void 
  bubble_up(vector<Mmsapt::TargetPhraseCollectionWrapper*>& v, size_t k)
  {
    if (k >= v.size()) return; 
    for (;k && (v[k]->tstamp < v[k/2]->tstamp); k /=2)
      {
  	std::swap(v[k],v[k/2]);
  	std::swap(v[k]->idx,v[k/2]->idx);
      }
  }

  void 
  bubble_down(vector<Mmsapt::TargetPhraseCollectionWrapper*>& v, size_t k)
  {
    for (size_t j = 2*(k+1); j <= v.size(); j = 2*((k=j)+1))
      {
	if (j == v.size() || (v[j-1]->tstamp < v[j]->tstamp)) --j;
	if (v[j]->tstamp >= v[k]->tstamp) break;
	std::swap(v[k],v[j]);
	v[k]->idx = k;
	v[j]->idx = j;
      }
  }

  void
  Mmsapt::
  decache(TargetPhraseCollectionWrapper* ptr) const
  {
    if (ptr->refCount || ptr->idx >= 0) return;
    
    timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    timespec r; clock_getres(CLOCK_MONOTONIC,&r);

    // if (t.tv_nsec < v[0]->tstamp.tv_nsec)
#if 0
    float delta = t.tv_sec - ptr->tstamp.tv_sec;
    cerr << "deleting old cache entry after "
	 << delta << " seconds."
	 << " clock resolution is " << r.tv_sec << ":" << r.tv_nsec 
	 << " at " << __FILE__ << ":" << __LINE__ << endl;
#endif
    tpc_cache_t::iterator m = m_cache.find(ptr->key);
    if (m != m_cache.end())
      if (m->second == ptr)
	m_cache.erase(m);
    delete ptr;
    --m_tpc_ctr;
  }
  

  Mmsapt::
  TargetPhraseCollectionWrapper*
  Mmsapt::
  encache(TargetPhraseCollectionWrapper* ptr) const
  {
    // Calling process must lock for thread safety!!
    if (!ptr) return NULL;
    ++ptr->refCount;
    ++m_tpc_ctr;
    clock_gettime(CLOCK_MONOTONIC, &ptr->tstamp);
    
    // update history
    if (m_history.capacity() > 1)
      {
	vector<TargetPhraseCollectionWrapper*>& v = m_history;
	if (ptr->idx >= 0) // ptr is already in history
	  { 
	    assert(ptr == v[ptr->idx]);
	    size_t k = 2 * (ptr->idx + 1);
	    if (k < v.size()) bubble_up(v,k--);
	    if (k < v.size()) bubble_up(v,k);
	  }
	else if (v.size() < v.capacity())
	  {
	    size_t k = ptr->idx = v.size();
	    v.push_back(ptr);
	    bubble_up(v,k);
	  }
	else 
	  {
	    v[0]->idx = -1;
	    decache(v[0]);
	    v[0] = ptr;
	    bubble_down(v,0);
	  }
      }
    return ptr;
  }

  bool
  Mmsapt::
  PrefixExists(Moses::Phrase const& phrase) const
  {
    if (phrase.GetSize() == 0) return false;
    vector<id_type> myphrase; 
    fillIdSeq(phrase,input_factor,*btfix.V1,myphrase);
    
    TSA<Token>::tree_iterator mfix(btfix.I1.get(),&myphrase[0],myphrase.size());
    if (mfix.size() == myphrase.size()) 
      {
	// cerr << phrase << " " << mfix.approxOccurrenceCount() << endl;
	return true;
      }

    sptr<imBitext<Token> > dyn;
    { // braces are needed for scoping mutex lock guard!
      boost::lock_guard<boost::mutex> guard(this->lock);
      dyn = btdyn;
    }
    assert(dyn);
    TSA<Token>::tree_iterator mdyn(dyn->I1.get());
    if (dyn->I1.get())
      {
	for (size_t i = 0; mdyn.size() == i && i < myphrase.size(); ++i)
	  mdyn.extend(myphrase[i]);
      }
    return mdyn.size() == myphrase.size();
  }

  void
  Mmsapt::
  Release(TargetPhraseCollection const* tpc) const
  {
    if (!tpc) return;
    boost::lock_guard<boost::mutex> guard(this->lock);
    TargetPhraseCollectionWrapper* ptr 
      = (reinterpret_cast<TargetPhraseCollectionWrapper*>
	 (const_cast<TargetPhraseCollection*>(tpc)));
    if (--ptr->refCount == 0 && ptr->idx < 0)
      decache(ptr);
#if 0
    cerr << ptr->refCount << " references at " 
	 << __FILE__ << ":" << __LINE__ 
	 << "; " << m_tpc_ctr << " TPC references still in circulation; "
	 << m_history.size() << " instances in history."
	 << endl;
#endif
  }

  bool
  Mmsapt::
  ProvidesPrefixCheck() const
  {
    return true;
  }

}
