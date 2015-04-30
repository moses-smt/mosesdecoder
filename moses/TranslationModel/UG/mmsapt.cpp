#ifdef HAVE_CURLPP
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#endif

#include "mmsapt.h"
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/tokenizer.hpp>
#include <boost/thread/locks.hpp>
#include <algorithm>
#include "util/exception.hh"
#include <set>

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
    : PhraseDictionary(description,line), ofactor(1,0), m_bias_log(NULL)
    , m_bias_loglevel(0)
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
    : PhraseDictionary(line, false)
    , m_bias_log(NULL)
    , m_bias_loglevel(0)
    , m_lr_func(NULL)
    , cache_key(((char*)this)+2)
    , context_key(((char*)this)+1)
      // , m_tpc_ctr(0)
    , ofactor(1,0)
  {
    init(line);
    setup_local_feature_functions();
    Register();
  }

  void
  Mmsapt::
  read_config_file(string fname, map<string,string>& param)
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

  bool Mmsapt::isLogVal(int i) const { return m_is_logval.at(i); }
  bool Mmsapt::isInteger(int i) const { return m_is_integer.at(i); }

  void Mmsapt::init(string const& line)
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
	m_bname = m->second;
	m = param.find("path");
	UTIL_THROW_IF2((m != param.end() && m->second != m_bname),
	 	       "Conflicting aliases for path:\n"
		       << "path=" << string(m->second) << "\n"
		       << "base=" << m_bname.c_str() );
      }
    else m_bname = param["path"];
    L1    = param["L1"];
    L2    = param["L2"];

    UTIL_THROW_IF2(m_bname.size() == 0, "Missing corpus base name at " << HERE);
    UTIL_THROW_IF2(L1.size() == 0, "Missing L1 tag at " << HERE);
    UTIL_THROW_IF2(L2.size() == 0, "Missing L2 tag at " << HERE);

    // set defaults for all parameters if not specified so far
    pair<string,string> dflt("input-factor","0");
    input_factor = atoi(param.insert(dflt).first->second.c_str());
    // shouldn't that be a string?

    dflt = pair<string,string> ("output-factor","0");
    output_factor = atoi(param.insert(dflt).first->second.c_str());
    ofactor.assign(1,output_factor);

    dflt = pair<string,string> ("smooth",".01");
    m_lbop_conf = atof(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string> ("lexalpha","0");
    m_lex_alpha = atof(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string> ("sample","1000");
    m_default_sample_size = atoi(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string>("workers","8");
    m_workers = atoi(param.insert(dflt).first->second.c_str());
    m_workers = min(m_workers,24UL);

    dflt = pair<string,string>("bias-loglevel","0");
    m_bias_loglevel = atoi(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string>("table-limit","20");
    m_tableLimit = atoi(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string>("cache","10000");
    m_cache_size = max(1000,atoi(param.insert(dflt).first->second.c_str()));
    m_cache.reset(new TPCollCache(m_cache_size));
    // m_history.reserve(hsize);
    // in plain language: cache size is at least 1000, and 10,000 by default
    // this cache keeps track of the most frequently used target
    // phrase collections even when not actively in use

    // Feature functions are initialized  in function Load();
    param.insert(pair<string,string>("pfwd",   "g"));
    param.insert(pair<string,string>("pbwd",   "g"));
    param.insert(pair<string,string>("logcnt", "0"));
    param.insert(pair<string,string>("coh",    "0"));
    param.insert(pair<string,string>("rare",   "1"));
    param.insert(pair<string,string>("prov",   "1"));

    poolCounts = true;

    // this is for pre-comuted sentence-level bias; DEPRECATED!
    if ((m = param.find("bias")) != param.end())
	m_bias_file = m->second;

    if ((m = param.find("bias-server")) != param.end())
	m_bias_server = m->second;

    if ((m = param.find("bias-logfile")) != param.end())
      {
	m_bias_logfile = m->second;
	if (m_bias_logfile == "/dev/stderr")
	  m_bias_log = &std::cerr;
	else if (m_bias_logfile == "/dev/stdout")
	  m_bias_log = &std::cout;
	else
	  {
	    m_bias_logger.reset(new ofstream(m_bias_logfile.c_str()));
	    m_bias_log = m_bias_logger.get();
	  }
      }

    if ((m = param.find("lr-func")) != param.end())
      m_lr_func_name = m->second;

    if ((m = param.find("extra")) != param.end())
      m_extra_data = m->second;

    dflt = pair<string,string>("tuneable","true");
    m_tuneable = Scan<bool>(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string>("feature-sets","standard");
    m_feature_set_names = Tokenize(param.insert(dflt).first->second.c_str(), ",");
    m = param.find("name");
    if (m != param.end()) m_name = m->second;

    // check for unknown parameters
    vector<string> known_parameters; known_parameters.reserve(50);
    known_parameters.push_back("L1");
    known_parameters.push_back("L2");
    known_parameters.push_back("Mmsapt");
    known_parameters.push_back("PhraseDictionaryBitextSampling");
    // alias for Mmsapt
    known_parameters.push_back("base"); // alias for path
    known_parameters.push_back("bias");
    known_parameters.push_back("bias-server");
    known_parameters.push_back("bias-logfile");
    known_parameters.push_back("bias-loglevel");
    known_parameters.push_back("cache");
    known_parameters.push_back("coh");
    known_parameters.push_back("config");
    known_parameters.push_back("extra");
    known_parameters.push_back("feature-sets");
    known_parameters.push_back("input-factor");
    known_parameters.push_back("lexalpha");
    // known_parameters.push_back("limit"); // replaced by "table-limit"
    known_parameters.push_back("logcnt");
    known_parameters.push_back("lr-func"); // associated lexical reordering function
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
    known_parameters.push_back("tuneable");
    known_parameters.push_back("unal");
    known_parameters.push_back("workers");
    sort(known_parameters.begin(),known_parameters.end());
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
  load_bias(string const fname)
  {
    m_bias = btfix.loadSentenceBias(fname);
  }

  void
  Mmsapt::
  load_extra_data(string bname, bool locking = true)
  {
    using namespace boost;
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

    scoped_ptr<boost::unique_lock<shared_mutex> > guard;
    if (locking) guard.reset(new boost::unique_lock<shared_mutex>(m_lock));
    btdyn = btdyn->add(text1,text2,symal);
    assert(btdyn);
    cerr << "Loaded " << btdyn->T1->size() << " sentence pairs" << endl;
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
  check_ff(string const ffname, float const xtra,
	   vector<sptr<pscorer> >* registry)
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
    Load(true);
  }

  void
  Mmsapt
  ::setup_local_feature_functions()
  {
    boost::unique_lock<boost::shared_mutex> lock(m_lock);
    // load feature sets
    BOOST_FOREACH(string const& fsname, m_feature_set_names)
      {
	// standard (default) feature set
	if (fsname == "standard")
	  {
	    // lexical scores
	    string lexfile = m_bname + L1 + "-" + L2 + ".lex";
	    sptr<PScoreLex1<Token> >
	      ff(new PScoreLex1<Token>(param["lex_alpha"],lexfile));
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
	  }

	// data source features (copies of phrase and word count specific to
	// this translation model)
	else if (fsname == "datasource")
	  {
	    sptr<PScorePC<Token> > ffpcnt(new PScorePC<Token>("pcnt"));
	    register_ff(ffpcnt,m_active_ff_common);
	    sptr<PScoreWC<Token> > ffwcnt(new PScoreWC<Token>("wcnt"));
	    register_ff(ffwcnt,m_active_ff_common);
	  }
      }
    // cerr << "Features: " << Join("|",m_feature_names) << endl;
    this->m_numScoreComponents = this->m_feature_names.size();
    this->m_numTuneableComponents  = this->m_numScoreComponents;
  }

  void
  Mmsapt::
  Load(bool with_checks)
  {
    // load feature functions (i.e., load underlying data bases, if any)
    BOOST_FOREACH(sptr<pscorer>& ff, m_active_ff_fix) ff->load();
    BOOST_FOREACH(sptr<pscorer>& ff, m_active_ff_dyn) ff->load();
    BOOST_FOREACH(sptr<pscorer>& ff, m_active_ff_common) ff->load();
#if 0
    if (with_checks)
      {
	UTIL_THROW_IF2(this->m_feature_names.size() != this->m_numScoreComponents,
		       "At " << HERE << ": number of feature values provided by "
		       << "Phrase table (" << this->m_feature_names.size()
		       << ") does not match number specified in Moses config file ("
		       << this->m_numScoreComponents << ")!\n";);
      }
#endif
    // Load corpora. For the time being, we can have one memory-mapped static
    // corpus and one in-memory dynamic corpus
    boost::unique_lock<boost::shared_mutex> lock(m_lock);

    btfix.m_num_workers = this->m_workers;
    btfix.open(m_bname, L1, L2);
    btfix.setDefaultSampleSize(m_default_sample_size);

    btdyn.reset(new imbitext(btfix.V1, btfix.V2, m_default_sample_size, m_workers));
    if (m_bias_file.size())
      load_bias(m_bias_file);

    if (m_extra_data.size())
      load_extra_data(m_extra_data, false);

#if 0
    // currently not used
    LexicalPhraseScorer2<Token>::table_t & COOC = calc_lex.scorer.COOC;
    typedef LexicalPhraseScorer2<Token>::table_t::Cell cell_t;
    wlex21.resize(COOC.numCols);
    for (size_t r = 0; r < COOC.numRows; ++r)
      for (cell_t const* c = COOC[r].start; c < COOC[r].stop; ++c)
	wlex21[c->id].push_back(r);
    COOCraw.open(m_bname + L1 + "-" + L2 + ".coc");
#endif
    assert(btdyn);
    // cerr << "LOADED " << HERE << endl;
  }

  void
  Mmsapt::
  add(string const& s1, string const& s2, string const& a)
  {
    vector<string> S1(1,s1);
    vector<string> S2(1,s2);
    vector<string> ALN(1,a);
    boost::unique_lock<boost::shared_mutex> guard(m_lock);
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
    tp->SetAlignTerm(pool.aln);
    tp->GetScoreBreakdown().Assign(this, fvals);
    tp->EvaluateInIsolation(src);

    if (m_lr_func)
      {
	LRModel::ModelType mdl = m_lr_func->GetModel().GetModelType();
	LRModel::Direction dir = m_lr_func->GetModel().GetDirection();
	sptr<Scores> scores(new Scores());
	pool.fill_lr_vec(dir, mdl, *scores);
	tp->SetExtraScores(m_lr_func, scores);
      }

    return tp;
  }

  void
  Mmsapt::
  GetTargetPhraseCollectionBatch(ttasksptr const& ttask,
				 const InputPathList &inputPathQueue) const
  {
    InputPathList::const_iterator iter;
    for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter)
      {
	InputPath &inputPath = **iter;
	const Phrase &phrase = inputPath.GetPhrase();
	PrefixExists(ttask, phrase); // launches parallel lookup
      }
    for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter)
      {
	InputPath &inputPath = **iter;
	const Phrase &phrase = inputPath.GetPhrase();
	const TargetPhraseCollection *targetPhrases
	  = this->GetTargetPhraseCollectionLEGACY(ttask,phrase);
	inputPath.SetTargetPhrases(*this, targetPhrases, NULL);
      }
  }

  TargetPhraseCollection const*
  Mmsapt::
  GetTargetPhraseCollectionLEGACY(const Phrase& src) const
  {
    UTIL_THROW2("Don't call me without the translation task.");
  }

  // This is not the most efficient way of phrase lookup!
  TargetPhraseCollection const*
  Mmsapt::
  GetTargetPhraseCollectionLEGACY(ttasksptr const& ttask, const Phrase& src) const
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
      boost::unique_lock<boost::shared_mutex> guard(m_lock);
      assert(btdyn);
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

    // do we have cached results for this phrase?
    uint64_t phrasekey = (mfix.size() == sphrase.size()
			  ? (mfix.getPid()<<1) : (mdyn.getPid()<<1)+1);

    // get context-specific cache of items previously looked up
    sptr<ContextScope> const& scope = ttask->GetScope();
    sptr<TPCollCache> cache = scope->get<TPCollCache>(cache_key);
    TPCollWrapper* ret = cache->get(phrasekey, dyn->revision());
    // TO DO: we should revise the revision mechanism: we take the length
    // of the dynamic bitext (in sentences) at the time the PT entry
    // was stored as the time stamp. For each word in the
    // vocabulary, we also store its most recent occurrence in the
    // bitext. Only if the timestamp of each word in the phrase is
    // newer than the timestamp of the phrase itself we must update
    // the entry.

    if (ret) return ret; // yes, was cached => DONE

    // OK: pt entry NOT found or NOT up to date
    // lookup and expansion could be done in parallel threads,
    // but ppdyn is probably small anyway
    // TO DO: have Bitexts return lists of PhrasePairs instead of pstats
    // no need to expand pstats at every single lookup again, especially
    // for btfix.
    sptr<pstats> sfix,sdyn;

    if (mfix.size() == sphrase.size()) sfix = btfix.lookup(ttask, mfix);
    if (mdyn.size() == sphrase.size()) sdyn = dyn->lookup(ttask, mdyn);

    vector<PhrasePair<Token> > ppfix,ppdyn;
    PhrasePair<Token>::SortByTargetIdSeq sort_by_tgt_id;
    if (sfix)
      {
	expand(mfix, btfix, *sfix, ppfix, m_bias_log);
	sort(ppfix.begin(), ppfix.end(),sort_by_tgt_id);
      }
    if (sdyn)
      {
	expand(mdyn, *dyn, *sdyn, ppdyn, m_bias_log);
	sort(ppdyn.begin(), ppdyn.end(),sort_by_tgt_id);
      }
    // now we have two lists of Phrase Pairs, let's merge them
    ret = new TPCollWrapper(dyn->revision(), phrasekey);
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

#if 1
    if (m_bias_log && m_lr_func)
      {
	typename PhrasePair<Token>::SortDescendingByJointCount sorter;
	sort(ppfix.begin(), ppfix.end(),sorter);
	BOOST_FOREACH(PhrasePair<Token> const& pp, ppfix)
	  {
	    if (&pp != &ppfix.front() && pp.joint <= 1) break;
	    pp.print(*m_bias_log,*btfix.V1, *btfix.V2, m_lr_func->GetModel());
	  }
      }
#endif


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

    cache->add(phrasekey, ret);
    return ret;
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
  CleanUpAfterSentenceProcessing(ttasksptr const& ttask)
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
  InitializeForInput(ttasksptr const& ttask)
  {
    sptr<ContextScope> const& scope = ttask->GetScope();
    sptr<ContextForQuery> context
      = scope->get<ContextForQuery>(&btfix, true);
    if (m_bias_server.size() && context->bias == NULL)
      { // we need to create the bias
	boost::unique_lock<boost::shared_mutex> lock(context->lock);
	string const& context_words = ttask->GetContextString();
	if (context_words.size())
	  {
	    if (m_bias_log)
	      {
		*m_bias_log << HERE << endl
			    << "BIAS LOOKUP CONTEXT: "
			    << context_words << endl;
		context->bias_log = m_bias_log;
	      }
	    context->bias
	      = btfix.SetupDocumentBias(m_bias_server, context_words, m_bias_log);
	    context->bias->loglevel = m_bias_loglevel;
	    context->bias->log = m_bias_log;
	  }
	if (!context->cache1) context->cache1.reset(new pstats::cache_t);
	if (!context->cache2) context->cache2.reset(new pstats::cache_t);
      }
    boost::unique_lock<boost::shared_mutex> mylock(m_lock);
    sptr<TPCollCache> localcache = scope->get<TPCollCache>(cache_key);
    if (!localcache)
      {
	if (context->bias) localcache.reset(new TPCollCache(m_cache_size));
	else localcache = m_cache;
	scope->set<TPCollCache>(cache_key, localcache);
      }

    if (m_lr_func_name.size() && m_lr_func == NULL)
      {
	FeatureFunction* lr = &FeatureFunction::FindFeatureFunction(m_lr_func_name);
	m_lr_func = dynamic_cast<LexicalReordering*>(lr);
	UTIL_THROW_IF2(lr == NULL, "FF " << m_lr_func_name
		       << " does not seem to be a lexical reordering function!");
	// todo: verify that lr_func implements a hierarchical reordering model
      }
  }

  // bool
  // Mmsapt::
  // PrefixExists(Moses::Phrase const& phrase) const
  // {
  //   return PrefixExists(phrase,NULL);
  // }

  bool
  Mmsapt::
  PrefixExists(ttasksptr const& ttask, Moses::Phrase const& phrase) const
  {
    if (phrase.GetSize() == 0) return false;
    vector<id_type> myphrase;
    fillIdSeq(phrase,input_factor,*btfix.V1,myphrase);

    TSA<Token>::tree_iterator mfix(btfix.I1.get(),&myphrase[0],myphrase.size());
    if (mfix.size() == myphrase.size())
      {
	btfix.prep(ttask, mfix);
	// cerr << phrase << " " << mfix.approxOccurrenceCount() << endl;
	return true;
      }

    sptr<imBitext<Token> > dyn;
    { // braces are needed for scoping lock!
      boost::unique_lock<boost::shared_mutex> guard(m_lock);
      dyn = btdyn;
    }
    assert(dyn);
    TSA<Token>::tree_iterator mdyn(dyn->I1.get());
    if (dyn->I1.get())
      {
	for (size_t i = 0; mdyn.size() == i && i < myphrase.size(); ++i)
	  mdyn.extend(myphrase[i]);
	// let's assume a uniform bias over the foreground corpus
	if (mdyn.size() == myphrase.size()) dyn->prep(ttask, mdyn);
      }
    return mdyn.size() == myphrase.size();
  }

  void
  Mmsapt
  ::Release(ttasksptr const& ttask, TargetPhraseCollection*& tpc) const
  {
    sptr<TPCollCache> cache = ttask->GetScope()->get<TPCollCache>(cache_key);
    TPCollWrapper* foo = static_cast<TPCollWrapper*>(tpc);
    if (cache) cache->release(foo);
    tpc = NULL;
  }

  bool Mmsapt
  ::ProvidesPrefixCheck() const { return true; }

  string const& Mmsapt
  ::GetName() const { return m_name; }

  // sptr<DocumentBias>
  // Mmsapt
  // ::setupDocumentBias(map<string,float> const& bias) const
  // {
  //   return btfix.SetupDocumentBias(bias);
  // }

  vector<float>
  Mmsapt
  ::DefaultWeights() const
  { return vector<float>(this->GetNumScoreComponents(), 1.); }

}
