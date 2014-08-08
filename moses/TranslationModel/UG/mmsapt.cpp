#include "mmsapt.h"
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/tokenizer.hpp>
#include <algorithm>
#include "moses/TranslationModel/UG/mm/ug_phrasepair.h"
#include "util/exception.hh"
#include <set>

namespace Moses
{
  using namespace bitext;
  using namespace std;
  using namespace boost;


  // uint64_t 
  // pack_phrasekey(uint64_t const shard_id, uint64_t const snt_id, 
  // 		 uint64_t const offset, uint64_t const len)
  // {
  //   uint64_t one = 1;
  //   //  8 bits - 256 shards
  //   // 13 bits - max offset
  //   // 11 bits - max len
  //   // 32 bits - max sentence id
  //   UTIL_TRHOW_IF2(shard_id >= 256, "[" << HERE << "] " 
  // 		   << "Sentence ID exceeds limit.");
  //   UTIL_THROW_IF2(snt_id >= 4294967296, "[" << HERE << "] " 
  // 		   << "Sentence ID exceeds limit.");
  //   UTIL_TRHOW_IF2(offset >= 8192, "[" << HERE << "]" 
  // 		   << "Phrase offset exceeds limit.");
  //   UTIL_TRHOW_IF2(offset >= 2048, "[" << HERE << "]" 
  // 		   << "Phrase length exceeds limit.");
  //   return ((shard_id<<56)+(snt_id<<24)+(offset<<11)+len);
  // }
  
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
  parseLine(string const& line, map<string,string> & param)
  {
    char_separator<char> sep("; ");
    tokenizer<char_separator<char> > tokens(line,sep);
    BOOST_FOREACH(string const& t,tokens)
      {
	size_t i = t.find_first_not_of(" =");
	size_t j = t.find_first_of(" =",i+1);
	size_t k = t.find_first_not_of(" =",j+1);
	UTIL_THROW_IF2(i == string::npos || k == string::npos,
		       "[" << HERE << "] "
		       << "Parameter specification error near '"
		       << t << "' in moses ini line\n"
		      << line);
	assert(i != string::npos);
	assert(k != string::npos);
	param[t.substr(i,j)] = t.substr(k);
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

  vector<string> const&
  Mmsapt::
  GetFeatureNames() const
  {
    return m_feature_names;
  }

  Mmsapt::
  Mmsapt(string const& line)
    : PhraseDictionary(line)
      // , m_lex_alpha(1.0)
      // , withLogCountFeatures(false)
      // , withCoherence(true)
      // , m_pfwd_features("g")
      // , m_pbwd_features("g")
      // , withPbwd(true)
      // , poolCounts(true)
    , ofactor(1,0)
    , m_tpc_ctr(0)
  {
    this->init(line);
  }

  void 
  Mmsapt::
  read_config_file(string fname,map<string,string>& param)
  {
    string line;
    ifstream config(fname.c_str());
    while (getline(config,line))
      {
	if (line[0] == '#') continue;
	char_separator<char> sep(" \t");
	tokenizer<char_separator<char> > tokens(line,sep);
	tokenizer<char_separator<char> >::const_iterator t = tokens.begin();
	if (t == tokens.end()) continue;
	string& foo = param[*t++];
	if (t == tokens.end() || foo.size()) continue; 
	// second condition: do not overwrite settings from the line in moses.ini
	UTIL_THROW_IF2(*t++ != "=" || t == tokens.end(), 
		       "Syntax error in Mmsapt config file '" << fname << "'.");
	for (foo = *t++; t != tokens.end(); foo += " " + *t++);
      }
  }

  void
  Mmsapt::
  register_ff(sptr<pscorer> const& ff, vector<sptr<pscorer> > & registry)
  {
    registry.push_back(ff);
    ff->setIndex(m_feature_names.size());
    for (int i = 0; i < ff->fcnt(); ++i)
      {
	m_feature_names.push_back(ff->fname(i));
	m_is_logval.push_back(ff->isLogVal(i));
	m_is_integer.push_back(ff->isIntegerValued(i));
      }
  }

  bool 
  Mmsapt::
  isLogVal(int i) const { return m_is_logval.at(i); }

  bool 
  Mmsapt::
  isInteger(int i) const { return m_is_integer.at(i); }

  void
  Mmsapt::
  init(string const& line)
  {
    map<string,string>::const_iterator m;
    parseLine(line,this->param);

    this->m_numScoreComponents = atoi(param["num-features"].c_str());
    
    m = param.find("config");
    if (m != param.end())
      read_config_file(m->second,param);

    m = param.find("base");
    if (m != param.end())
      {
	bname = m->second; 
	m = param.find("path");
	UTIL_THROW_IF2((m != param.end() && m->second != bname),
	 	       "Conflicting aliases for path:\n" 
		       << "path=" << string(m->second) << "\n"
		       << "base=" << bname.c_str() );
      }
    else bname = param["path"];
    L1    = param["L1"];
    L2    = param["L2"];
    
    UTIL_THROW_IF2(bname.size() == 0, "Missing corpus base name at " << HERE);
    UTIL_THROW_IF2(L1.size() == 0, "Missing L1 tag at " << HERE);
    UTIL_THROW_IF2(L2.size() == 0, "Missing L2 tag at " << HERE);

    // set defaults for all parameters if not specified so far
    pair<string,string> dflt("input-factor","0");
    input_factor = atoi(param.insert(dflt).first->second.c_str());
    // shouldn't that be a string?
    
    dflt = pair<string,string> ("smooth",".01");
    m_lbop_conf = atof(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string> ("lexalpha","0");
    m_lex_alpha = atof(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string> ("sample","1000");
    m_default_sample_size = atoi(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string>("workers","8");
    m_workers = atoi(param.insert(dflt).first->second.c_str());
    m_workers = min(m_workers,24UL);

    
    dflt = pair<string,string>("table-limit","20");
    m_tableLimit = atoi(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string>("cache","10000");
    size_t hsize = max(1000,atoi(param.insert(dflt).first->second.c_str()));
    m_history.reserve(hsize);
    // in plain language: cache size is at least 1000, and 10,000 by default
    // this cache keeps track of the most frequently used target phrase collections
    // even when not actively in use

    // Feature functions are initialized  in function Load();
    param.insert(pair<string,string>("pfwd",   "g"));  
    param.insert(pair<string,string>("pbwd",   "g"));  
    param.insert(pair<string,string>("logcnt", "0")); 
    param.insert(pair<string,string>("coh",    "0")); 
    param.insert(pair<string,string>("rare",   "1")); 
    param.insert(pair<string,string>("prov",   "1")); 
    
    poolCounts = true;
    
    if ((m = param.find("extra")) != param.end()) 
      extra_data = m->second;

    // check for unknown parameters
    vector<string> known_parameters; known_parameters.reserve(50);
    known_parameters.push_back("L1");
    known_parameters.push_back("L2");
    known_parameters.push_back("Mmsapt");
    known_parameters.push_back("base"); // alias for path
    known_parameters.push_back("cache");
    known_parameters.push_back("coh");
    known_parameters.push_back("config");
    known_parameters.push_back("extra");
    known_parameters.push_back("input-factor");
    known_parameters.push_back("lexalpha");
    // known_parameters.push_back("limit"); // replaced by "table-limit"
    known_parameters.push_back("logcnt");
    known_parameters.push_back("name");
    known_parameters.push_back("num-features");
    known_parameters.push_back("output-factor");
    known_parameters.push_back("path"); 
    known_parameters.push_back("pbwd");
    known_parameters.push_back("pfwd");
    known_parameters.push_back("prov");
    known_parameters.push_back("rare");
    known_parameters.push_back("sample");
    known_parameters.push_back("smooth");
    known_parameters.push_back("table-limit");
    known_parameters.push_back("unal");
    known_parameters.push_back("workers");
    for (map<string,string>::iterator m = param.begin(); m != param.end(); ++m)
      {
	UTIL_THROW_IF2(!binary_search(known_parameters.begin(),
				      known_parameters.end(), m->first),
		       HERE << ": Unknown parameter specification for Mmsapt: " 
		       << m->first);
      }
  }

  void
  Mmsapt::
  load_extra_data(string bname, bool locking = true)
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

    boost::scoped_ptr<lock_guard<mutex> > guard;
    if (locking) guard.reset(new lock_guard<mutex>(this->lock));
    btdyn = btdyn->add(text1,text2,symal);
    assert(btdyn);
    // cerr << "Loaded " << btdyn->T1->size() << " sentence pairs" << endl;
  }

  template<typename fftype>
  void
  Mmsapt::
  check_ff(string const ffname, vector<sptr<pscorer> >* registry)
  {
    string const& spec = param[ffname];
    if (spec == "" || spec == "0") return;
    if (registry)
      {
	sptr<fftype> ff(new fftype(spec));
	register_ff(ff, *registry);
      }
    else if (spec[spec.size()-1] == '+') // corpus specific
      {
	sptr<fftype> ff(new fftype(spec));
	register_ff(ff, m_active_ff_fix);
	ff.reset(new fftype(spec));
	register_ff(ff, m_active_ff_dyn);
      }
    else 
      {
	sptr<fftype> ff(new fftype(spec));
	register_ff(ff, m_active_ff_common);
      }
  }

  template<typename fftype>
  void
  Mmsapt::
  check_ff(string const ffname, float const xtra, vector<sptr<pscorer> >* registry)
  {
    string const& spec = param[ffname];
    if (spec == "" || spec == "0") return;
    if (registry)
      {
	sptr<fftype> ff(new fftype(xtra,spec));
	register_ff(ff, *registry);
      }
    else if (spec[spec.size()-1] == '+') // corpus specific
      {
	sptr<fftype> ff(new fftype(xtra,spec));
	register_ff(ff, m_active_ff_fix);
	ff.reset(new fftype(xtra,spec));
	register_ff(ff, m_active_ff_dyn);
      }
    else 
      {
	sptr<fftype> ff(new fftype(xtra,spec));
	register_ff(ff, m_active_ff_common);
      }
  }

  // void
  // Mmsapt::
  // add_corpus_specific_features(vector<sptr<pscorer > >& registry)
  // {
  //   check_ff<PScorePbwd<Token> >("pbwd",m_lbop_conf,registry);
  //   check_ff<PScoreLogCnt<Token> >("logcnt",registry);
  // }

  void
  Mmsapt::
  Load()
  {
    lock_guard<mutex> guard(this->lock);

    // can load only once
    // UTIL_THROW_IF2(shards.size(),"Mmsapt is already loaded at " << HERE);

    // lexical scores 
    string lexfile = bname + L1 + "-" + L2 + ".lex";
    sptr<PScoreLex1<Token> > ff(new PScoreLex1<Token>(param["lex_alpha"],lexfile));
    register_ff(ff,m_active_ff_common);

    // these are always computed on pooled data
    check_ff<PScoreRareness<Token> > ("rare", &m_active_ff_common);
    check_ff<PScoreUnaligned<Token> >("unal", &m_active_ff_common);
    check_ff<PScoreCoherence<Token> >("coh",  &m_active_ff_common);
    
    // for these ones either way is possible (specification ends with '+' 
    // if corpus-specific 
    check_ff<PScorePfwd<Token> >("pfwd", m_lbop_conf);
    check_ff<PScorePbwd<Token> >("pbwd", m_lbop_conf);
    check_ff<PScoreLogCnt<Token> >("logcnt");

    // These are always corpus-specific
    check_ff<PScoreProvenance<Token> >("prov", &m_active_ff_fix);
    check_ff<PScoreProvenance<Token> >("prov", &m_active_ff_dyn);

    UTIL_THROW_IF2(this->m_feature_names.size() != this->m_numScoreComponents,
		   "At " << HERE << ": number of feature values provided by "
		   << "Phrase table (" << this->m_feature_names.size()
		   << ") does not match number specified in Moses config file ("
		   << this->m_numScoreComponents << ")!\n";);

    // Load corpora. For the time being, we can have one memory-mapped static
    // corpus and one in-memory dynamic corpus
    // sptr<mmbitext> btfix(new mmbitext());
    btfix.num_workers = this->m_workers;
    btfix.open(bname, L1, L2);
    btfix.setDefaultSampleSize(m_default_sample_size);
    // shards.push_back(btfix);
    
    btdyn.reset(new imbitext(btfix.V1, btfix.V2, m_default_sample_size));
    btdyn->num_workers = this->m_workers;
    if (extra_data.size()) 
      load_extra_data(extra_data,false);
    
#if 0
    // currently not used
    LexicalPhraseScorer2<Token>::table_t & COOC = calc_lex.scorer.COOC;
    typedef LexicalPhraseScorer2<Token>::table_t::Cell cell_t;
    wlex21.resize(COOC.numCols);
    for (size_t r = 0; r < COOC.numRows; ++r)
      for (cell_t const* c = COOC[r].start; c < COOC[r].stop; ++c)
	wlex21[c->id].push_back(r);
    COOCraw.open(bname + L1 + "-" + L2 + ".coc");
#endif
    
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
  mkTPhrase(Phrase const& src,
	    PhrasePair<Token>* fix, 
	    PhrasePair<Token>* dyn, 
	    sptr<Bitext<Token> > const& dynbt) const
  {
    UTIL_THROW_IF2(!fix && !dyn, HERE << 
		   ": Can't create target phrase from nothing.");
    vector<float> fvals(this->m_numScoreComponents);
    PhrasePair<Token> pool = fix ? *fix : *dyn;
    if (fix) 
      {
	BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_fix)
	  (*ff)(btfix, *fix, &fvals);
      }
    if (dyn)
      {
	BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_dyn)
	  (*ff)(*dynbt, *dyn, &fvals);
      }
    
    if (fix && dyn) { pool += *dyn; }
    else if (fix)
      {
	PhrasePair<Token> zilch; zilch.init();
	TSA<Token>::tree_iterator m(dynbt->I2.get(), fix->start2, fix->len2);
	if (m.size() == fix->len2)
	  zilch.raw2 = m.approxOccurrenceCount();
	pool += zilch;
	BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_dyn)
	  (*ff)(*dynbt, ff->allowPooling() ? pool : zilch, &fvals);
      }
    else if (dyn)
      {
	PhrasePair<Token> zilch; zilch.init();
	TSA<Token>::tree_iterator m(btfix.I2.get(), dyn->start2, dyn->len2);
	if (m.size() == dyn->len2)
	  zilch.raw2 = m.approxOccurrenceCount();
	pool += zilch;
	BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_fix)
	  (*ff)(*dynbt, ff->allowPooling() ? pool : zilch, &fvals);
      }
    if (fix) 
      {
 	BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_common)
	  (*ff)(btfix, pool, &fvals);
      }
    else
      {
 	BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_common)
	  (*ff)(*dynbt, pool, &fvals);
      }
    TargetPhrase* tp = new TargetPhrase(this);
    Token const* x = fix ? fix->start2 : dyn->start2;
    uint32_t len = fix ? fix->len2 : dyn->len2;
    for (uint32_t k = 0; k < len; ++k, x = x->next())
      {
	StringPiece wrd = (*(btfix.V2))[x->id()];
	Word w; w.CreateFromString(Output,ofactor,wrd,false);
	tp->AddWord(w);
      }
    tp->GetScoreBreakdown().Assign(this, fvals);
    tp->EvaluateInIsolation(src);
    return tp;
  }

  // TargetPhrase* 
  // Mmsapt::
  // mkTPhrase(Phrase        const& src, 
  // 		     Bitext<Token> const& bt, 
  // 		     PhrasePair    const& pp) const
  // {
  //   Word w; uint32_t sid,off,len;    
  //   TargetPhrase* tp = new TargetPhrase();
  //   parse_pid(pp.p2, sid, off, len);
  //   Token const* x = bt.T2->sntStart(sid) + off;
  //   for (uint32_t k = 0; k < len; ++k)
  //     {
  // 	// cerr << (*bt.V2)[x[k].id()] << " at " << __FILE__ << ":" << __LINE__ << endl;
  // 	StringPiece wrd = (*bt.V2)[x[k].id()];
  // 	// if ((off+len) > bt.T2->sntLen(sid))
  // 	// cerr << off << ";" << len << " " << bt.T2->sntLen(sid) << endl;
  // 	assert(off+len <= bt.T2->sntLen(sid));
  // 	w.CreateFromString(Output,ofactor,wrd,false);
  // 	tp->AddWord(w);
  //     }
  //   tp->GetScoreBreakdown().Assign(this, pp.fvals);
  //   tp->Evaluate(src);
  //   return tp;
  // }

  // // process phrase stats from a single parallel corpus
  // void
  // Mmsapt::
  // process_pstats
  // (Phrase   const& src,
  //  uint64_t const  pid1, 
  //  pstats   const& stats, 
  //  Bitext<Token> const & bt, 
  //  TargetPhraseCollection* tpcoll
  //  ) const
  // {
  //   PhrasePair pp;   
  //   pp.init(pid1, stats, this->m_numScoreComponents);
  //   pstats::trg_map_t::const_iterator t;
  //   for (t = stats.trg.begin(); t != stats.trg.end(); ++t)
  //     {
  //  	pp.update(t->first,t->second);
  // 	BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_fix)
  // 	  (*ff)(bt,pp);
  // 	BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_common)
  // 	  (*ff)(bt,pp);
  // 	tpcoll->Add(mkTPhrase(src,bt,pp));
  //     }
  // }

  // void
  // Mmsapt::
  // ScorePPfix(PhrasePair& pp) const
  // {
  //   BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_fix)
  //     (*ff)(btfix,pp);
  //   BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_common)
  //     (*ff)(btfix,pp);
  // }

//   // process phrase stats from a single parallel corpus
//   bool
//   Mmsapt::
//   pool_pstats(Phrase   const& src,
// 	      uint64_t const  pid1a, 
// 	      pstats        * statsa, 
// 	      Bitext<Token> const & bta,
// 	      uint64_t const  pid1b, 
// 	      pstats   const* statsb, 
// 	      Bitext<Token> const & btb,
// 	      TargetPhraseCollection* tpcoll) const
//   {
//     PhrasePair pp;
//     if (statsa && statsb)
//       pp.init(pid1b, *statsa, *statsb, this->m_numScoreComponents);
//     else if (statsa)
//       pp.init(pid1a, *statsa, this->m_numScoreComponents);
//     else if (statsb)
//       pp.init(pid1b, *statsb, this->m_numScoreComponents);
//     else return false; // throw "no stats for pooling available!";

//     pstats::trg_map_t::const_iterator b;
//     pstats::trg_map_t::iterator a;
//     if (statsb)
//       {
// 	for (b = statsb->trg.begin(); b != statsb->trg.end(); ++b)
// 	  {
// 	    uint32_t sid,off,len;    
// 	    parse_pid(b->first, sid, off, len);
// 	    Token const* x = btb.T2->sntStart(sid) + off;
// 	    TSA<Token>::tree_iterator m(bta.I2.get(),x,x+len);
// 	    if (m.size() == len) 
// 	      {
// 		;
// 		if (statsa && ((a = statsa->trg.find(m.getPid())) 
// 			       != statsa->trg.end()))
// 		  {
// 		    pp.update(b->first,a->second,b->second);
// 		    a->second.invalidate();
// 		  }
// 		else 
// 		  pp.update(b->first,m.approxOccurrenceCount(),
// 			    b->second);
// 	      }
// 	    else pp.update(b->first,b->second);
// 	    BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_fix)
// 	      (*ff)(btb,pp);
// 	    BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_common)
// 	      (*ff)(btb,pp);
// 	    tpcoll->Add(mkTPhrase(src,btb,pp));
// 	  }
//       }
//     if (!statsa) return statsb != NULL;
//     for (a = statsa->trg.begin(); a != statsa->trg.end(); ++a)
//       {
// 	uint32_t sid,off,len;
// 	if (!a->second.valid()) continue;
// 	parse_pid(a->first, sid, off, len);
// 	if (btb.T2)
// 	  {
// 	    Token const* x = bta.T2->sntStart(sid) + off;
// 	    TSA<Token>::tree_iterator m(btb.I2.get(), x, len);
// 	    if (m.size() == len) 
// 	      pp.update(a->first,m.approxOccurrenceCount(),a->second);
// 	    else 
// 	      pp.update(a->first,a->second);
// 	  }
// 	else pp.update(a->first,a->second);
// #if 0
// 	// jstats const& j = a->second;
// 	cerr << bta.T1->pid2str(bta.V1.get(),pp.p1) << " ::: " 
// 	     << bta.T2->pid2str(bta.V2.get(),pp.p2) << endl;
// 	cerr << pp.raw1 << " " << pp.sample1 << " " << pp.good1 << " " 
// 	     << pp.joint << " " << pp.raw2 << endl;
// #endif

// 	UTIL_THROW_IF2(pp.raw2 == 0, 
// 		       "OOPS" << bta.T1->pid2str(bta.V1.get(),pp.p1) << " ::: " 
// 		       << bta.T2->pid2str(bta.V2.get(),pp.p2) << ": "
// 		       << pp.raw1 << " " << pp.sample1 << " " 
// 		       << pp.good1 << " " << pp.joint << " " 
// 		       << pp.raw2);
// 	BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_fix)
// 	  (*ff)(bta,pp);
// 	BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_common)
// 	  (*ff)(bta,pp);
// 	tpcoll->Add(mkTPhrase(src,bta,pp));
//       }
//     return true;
//   }



  
  // // process phrase stats from a single parallel corpus
  // bool
  // Mmsapt::
  // combine_pstats
  // (Phrase   const& src,
  //  uint64_t const  pid1a, pstats      * statsa, Bitext<Token> const & bta,
  //  uint64_t const  pid1b, pstats const* statsb, Bitext<Token> const & btb,
  //  TargetPhraseCollection* tpcoll) const
  // {
  //   if (!statsa && !statsb) return false; 

  //   PhrasePair ppfix,ppdyn,pool; Word w;
  //   // ppfix: counts from btfix
  //   // ppdyn: counts from btdyn
  //   // pool: pooled counts from both

  //   pstats::trg_map_t::const_iterator b;
  //   pstats::trg_map_t::iterator a;

    
  //   set<uint64_t> check;
  //   if (statsb)
  //     {
  // 	ppdyn.init(pid1b,*statsb,this->m_numScoreComponents);
  // 	if (statsa)
  // 	  {
  // 	    pool.init(pid1b, *statsa, *statsb, 0);
  // 	    ppfix.init(pid1a,*statsa, 0);
  // 	  }
  // 	else 
  // 	  {
  // 	    pool.init(pid1b, *statsb,0);
  // 	    ppfix.init();
  // 	  }
	
  // 	for (b = statsb->trg.begin(); b != statsb->trg.end(); ++b)
  // 	  {
  // 	    ppdyn.update(b->first,b->second);
  // 	    BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_dyn)
  // 	      (*ff)(btb,ppdyn);
	    
  // 	    uint32_t sid,off,len;    
  // 	    parse_pid(b->first, sid, off, len);
  // 	    Token const* x = btb.T2->sntStart(sid) + off;
  // 	    TSA<Token>::tree_iterator m(bta.I2.get(),x,len);
	    
  // 	    Token const* y = m.getToken(0);
  // 	    for (size_t i = 0; i < len; ++i)
  // 	      cout << x[i].id() << " " << endl;
  // 	    for (size_t i = 0; i < m.size(); ++i)
  // 	      cout << y[i].id() << " " << endl;
	    
  // 	    if (statsa && m.size() &&  
  // 		((a = statsa->trg.find(m.getPid())) != statsa->trg.end()))
  // 	      { // i.e., phrase pair found also in btfix
  // 		ppfix.update(a->first,a->second);
  // 		pool.update(b->first, b->second, a->second);
  // 		BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_fix)
  // 		  (*ff)(bta, ppfix, &ppdyn.fvals);
  // 		check.insert(a->first); 
  // 	      }
  // 	    else // phrase pair was not found in btfix
  // 	      {
  // 		if (m.size()) // ... but the source phrase was  
  // 		  {
  // 		    pool.update(b->first, m.approxOccurrenceCount(), b->second);
  // 		    ppfix.update(b->first,m.approxOccurrenceCount());
  // 		  }
  // 		else // ... and not even the source phrase 
  // 		  {
  // 		    pool.update(b->first, b->second);
  // 		    ppfix.update(b->first,0);
  // 		  }		    
  // 		BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_fix)
  // 		  (*ff)(btb, ff->allowPooling() ? pool : ppfix, &ppdyn.fvals);
  // 	      }
  // 	    BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_common)
  // 	      (*ff)(btb, pool, &ppdyn.fvals);
  // 	    tpcoll->Add(mkTPhrase(src,btb,ppdyn));
  // 	  }
  //     }

  //   // now deal with all phraise pairs that are ONLY in btfix
  //   // (the ones that are in both were dealt with above)
  //   if (statsa)
  //     {
  // 	ppfix.init(pid1a, *statsa, this->m_numScoreComponents);
  // 	pool.init(pid1a,  *statsa, 0);
  // 	ppdyn.init();
  // 	for (a = statsa->trg.begin(); a != statsa->trg.end(); ++a)
  // 	  {
  // 	    if (check.find(a->first) != check.end()) 
  // 	      continue;

  // 	    ppfix.update(a->first, a->second);
  // 	    BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_fix)
  // 	      (*ff)(bta, ppfix);
	    
  // 	    if (btb.I2)
  // 	      {
  // 		uint32_t sid,off,len;    
  // 		parse_pid(a->first, sid, off, len);
  // 		Token const* x = bta.T2->sntStart(sid) + off;
  // 		TSA<Token>::tree_iterator m(btb.I2.get(), x, len);
  // 		if (m.size())
  // 		  pool.update(a->first, m.approxOccurrenceCount(), a->second);
  // 		else
  // 		  pool.update(a->first, a->second);
  // 	      }
  // 	    else pool.update(a->first, a->second);
  // 	    BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_dyn)
  // 	      (*ff)(btb, ff->allowPooling() ? pool : ppdyn, &ppfix.fvals);
  // 	    BOOST_FOREACH(sptr<pscorer> const& ff, m_active_ff_common)
  // 	      (*ff)(bta, pool, &ppfix.fvals);
  // 	    if (ppfix.p2)
  // 	      tpcoll->Add(mkTPhrase(src, bta, ppfix));
  // 	  }
  //     }
  //   return true;
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
  
  template<typename Token>
  void 
  expand(typename Bitext<Token>::iter const& m, 
	 Bitext<Token> const& bt, 
	 pstats const& ps, vector<PhrasePair<Token> >& dest)
  {
    dest.reserve(ps.trg.size());
    PhrasePair<Token> pp;
    pp.init(m.getToken(0), m.size(), &ps, 0);
    // cout << HERE << " " << toString(*(bt.V1),pp.start1,pp.len1) << endl;
    pstats::trg_map_t::const_iterator a;
    for (a = ps.trg.begin(); a != ps.trg.end(); ++a)
      {
	uint32_t sid,off,len;
	parse_pid(a->first, sid, off, len);
	pp.update(bt.T2->sntStart(sid)+off, len, a->second);
	dest.push_back(pp);
      }
    typename PhrasePair<Token>::SortByTargetIdSeq sorter;
    sort(dest.begin(), dest.end(),sorter);
#if 0
    BOOST_FOREACH(PhrasePair<Token> const& p, dest)
      cout << toString (*bt.V1,p.start1,p.len1) << " ::: " 
	   << toString (*bt.V2,p.start2,p.len2) << " " 
	   << p.joint << endl;
#endif
  }

  // This is not the most efficient way of phrase lookup! 
  TargetPhraseCollection const* 
  Mmsapt::
  GetTargetPhraseCollectionLEGACY(const Phrase& src) const
  {
    // map from Moses Phrase to internal id sequence
    vector<id_type> sphrase; 
    fillIdSeq(src,input_factor,*(btfix.V1),sphrase);
    if (sphrase.size() == 0) return NULL;
    
    // Reserve a local copy of the dynamic bitext in its current form. /btdyn/
    // is set to a new copy of the dynamic bitext every time a sentence pair
    // is added. /dyn/ keeps the old bitext around as long as we need it.
    sptr<imBitext<Token> > dyn;
    { // braces are needed for scoping mutex lock guard!
      boost::lock_guard<boost::mutex> guard(this->lock);
      dyn = btdyn;
    }
    assert(dyn);

    // lookup phrases in both bitexts
    TSA<Token>::tree_iterator mfix(btfix.I1.get(), &sphrase[0], sphrase.size());
    TSA<Token>::tree_iterator mdyn(dyn->I1.get());
    if (dyn->I1.get())
      for (size_t i = 0; mdyn.size() == i && i < sphrase.size(); ++i)
	mdyn.extend(sphrase[i]);

#if 0
    cerr << src << endl;
    cerr << mfix.size() << ":" << mfix.getPid() << " "
	 << mdyn.size() << " " << mdyn.getPid() << endl;
#endif

    if (mdyn.size() != sphrase.size() && mfix.size() != sphrase.size()) 
      return NULL; // phrase not found in either bitext

    // cache lookup:
    uint64_t phrasekey = (mfix.size() == sphrase.size() ? (mfix.getPid()<<1) 
			  : (mdyn.getPid()<<1)+1);
    size_t revision = dyn->revision();
    {
      boost::lock_guard<boost::mutex> guard(this->lock);
      tpc_cache_t::iterator c = m_cache.find(phrasekey);
      // TO DO: we should revise the revision mechanism: we take the length
      // of the dynamic bitext (in sentences) at the time the PT entry
      // was stored as the time stamp. For each word in the
      // vocabulary, we also store its most recent occurrence in the
      // bitext. Only if the timestamp of each word in the phrase is
      // newer than the timestamp of the phrase itself we must update 
      // the entry. 
      if (c != m_cache.end() && c->second->revision == revision)
	return encache(c->second);
    }
    
    // OK: pt entry not found or not up to date
    // lookup and expansion could be done in parallel threds, 
    // but ppdyn is probably small anyway
    // TO DO: have Bitexts return lists of PhrasePairs instead of pstats
    // no need to expand pstats at every single lookup again, especially 
    // for btfix.
    sptr<pstats> sfix,sdyn;
    if (mfix.size() == sphrase.size()) sfix = btfix.lookup(mfix);
    if (mdyn.size() == sphrase.size()) sdyn = dyn->lookup(mdyn);

    vector<PhrasePair<Token> > ppfix,ppdyn;
    if (sfix) expand(mfix, btfix, *sfix, ppfix);
    if (sdyn) expand(mdyn, *dyn, *sdyn, ppdyn);
    
    // now we have two lists of Phrase Pairs, let's merge them
    TargetPhraseCollectionWrapper* ret;
    ret = new TargetPhraseCollectionWrapper(revision,phrasekey);
    PhrasePair<Token>::SortByTargetIdSeq sorter;
    size_t i = 0; size_t k = 0;
    while (i < ppfix.size() && k < ppdyn.size())
      {
	int cmp = sorter.cmp(ppfix[i], ppdyn[k]);
	if      (cmp  < 0) ret->Add(mkTPhrase(src,&ppfix[i++],NULL,dyn));
	else if (cmp == 0) ret->Add(mkTPhrase(src,&ppfix[i++],&ppdyn[k++],dyn));
	else               ret->Add(mkTPhrase(src,NULL,&ppdyn[k++],dyn));
      }
    while (i < ppfix.size()) ret->Add(mkTPhrase(src,&ppfix[i++],NULL,dyn));
    while (k < ppdyn.size()) ret->Add(mkTPhrase(src,NULL,&ppdyn[k++],dyn));
    if (m_tableLimit) ret->Prune(true, m_tableLimit);
    else ret->Prune(true,ret->GetSize());
#if 0
    if (combine_pstats(src, 
		       mfix.getPid(), sfix.get(), btfix, 
		       mdyn.getPid(), sdyn.get(),  *dyn, ret))
      {
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
#endif

    // put the result in the cache and return
    boost::lock_guard<boost::mutex> guard(this->lock);
    m_cache[phrasekey] = ret;
    return encache(ret);
  }

  size_t 
  Mmsapt::
  SetTableLimit(size_t limit)
  {
    std::swap(m_tableLimit,limit);
    return limit;
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
	btfix.prep(mfix);
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
	if (mdyn.size() == myphrase.size()) dyn->prep(mdyn);
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
