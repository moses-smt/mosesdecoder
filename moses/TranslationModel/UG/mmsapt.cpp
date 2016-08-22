// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-

#include "mmsapt.h"
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/tokenizer.hpp>
#include <boost/thread/locks.hpp>
#include <algorithm>
#include "util/exception.hh"
#include <set>
#include "util/usage.hh"

namespace Moses
{
  using namespace sapt;
  using namespace std;
  using namespace boost;

  void
  fillIdSeq(Phrase const& mophrase, std::vector<FactorType> const& ifactors,
            TokenIndex const& V, vector<id_type>& dest)
  {
    dest.resize(mophrase.GetSize());
    for (size_t i = 0; i < mophrase.GetSize(); ++i)
      {
        // Factor const* f = mophrase.GetFactor(i,ifactor);
        dest[i] = V[mophrase.GetWord(i).GetString(ifactors, false)]; // f->ToString()];
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

  vector<string> const&
  Mmsapt::
  GetFeatureNames() const
  {
    return m_feature_names;
  }

  Mmsapt::
  Mmsapt(string const& line)
    : PhraseDictionary(line, false)
    , btfix(new mmbitext)
    , m_bias_log(NULL)
    , m_bias_loglevel(0)
#ifndef NO_MOSES
    , m_lr_func(NULL)
#endif
    , m_sampling_method(random_sampling)
    , bias_key(((char*)this)+3)
    , cache_key(((char*)this)+2)
    , context_key(((char*)this)+1)
    , m_track_coord(false)
      // , m_tpc_ctr(0)
      // , m_ifactor(1,0)
      // , m_ofactor(1,0)
  {
    init(line);
    setup_local_feature_functions();
    // Set features used for scoring extracted phrases:
    // * Use all features that can operate on input factors and this model's
    //   output factor
    // * Don't use features that depend on generation steps that won't be run
    //   yet at extract time
    SetFeaturesToApply();
    // Register();
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
  register_ff(SPTR<pscorer> const& ff, vector<SPTR<pscorer> > & registry)
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

  void 
  Mmsapt::
  parse_factor_spec(std::vector<FactorType>& flist, std::string const key)
  {
    pair<string,string> dflt(key, "0");
    string factors = this->param.insert(dflt).first->second;
    size_t p = 0, q = factors.find(',');
    for (; q < factors.size(); q = factors.find(',', p=q+1))
      flist.push_back(atoi(factors.substr(p, q-p).c_str()));
    flist.push_back(atoi(factors.substr(p).c_str()));
  }
  

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
    parse_factor_spec(m_ifactor,"input-factor");
    parse_factor_spec(m_ofactor,"output-factor");

    // Masks for available factors that inform SetFeaturesToApply
    m_inputFactors = FactorMask(m_ifactor);
    m_outputFactors = FactorMask(m_ofactor);

    pair<string,string> dflt = pair<string,string> ("smooth",".01");
    m_lbop_conf = atof(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string> ("lexalpha","0");
    m_lex_alpha = atof(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string> ("sample","1000");
    m_default_sample_size = atoi(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string> ("min-sample","0");
    m_min_sample_size = atoi(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string>("workers","0");
    m_workers = atoi(param.insert(dflt).first->second.c_str());
    if (m_workers == 0) m_workers = StaticData::Instance().ThreadCount();
    else m_workers = min(m_workers,size_t(boost::thread::hardware_concurrency()));
    
    dflt = pair<string,string>("bias-loglevel","0");
    m_bias_loglevel = atoi(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string>("table-limit","20");
    m_tableLimit = atoi(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string>("cache","100000");
    m_cache_size = max(10000,atoi(param.insert(dflt).first->second.c_str()));

    m_cache.reset(new TPCollCache(m_cache_size));
    // m_history.reserve(hsize);
    // in plain language: cache size is at least 1000, and 10,000 by default
    // this cache keeps track of the most frequently used target
    // phrase collections even when not actively in use

    // Feature functions are initialized  in function Load();
    param.insert(pair<string,string>("pfwd",   "g"));
    param.insert(pair<string,string>("pbwd",   "g"));
    param.insert(pair<string,string>("lenrat", "1"));
    param.insert(pair<string,string>("rare",   "1"));
    param.insert(pair<string,string>("logcnt", "0"));
    param.insert(pair<string,string>("coh",    "0"));
    param.insert(pair<string,string>("prov",   "0"));
    param.insert(pair<string,string>("cumb",   "0"));

    poolCounts = true;

    // this is for pre-comuted sentence-level bias; DEPRECATED!
    if ((m = param.find("bias")) != param.end())
      m_bias_file = m->second;

    if ((m = param.find("bias-server")) != param.end())
      m_bias_server = m->second;
    if (m_bias_loglevel)
      {
        dflt = pair<string,string>("bias-logfile","/dev/stderr");
        param.insert(dflt);
      }
    if ((m = param.find("bias-logfile")) != param.end())
      {
        m_bias_logfile = m->second;
        if (m_bias_logfile == "/dev/stderr")
          m_bias_log = &std::cerr;
        else if (m_bias_logfile == "/dev/stdout")
          m_bias_log = &std::cout;
        else
          {
            m_bias_logger.reset(new std::ofstream(m_bias_logfile.c_str()));
            m_bias_log = m_bias_logger.get();
          }
      }

    if ((m = param.find("lr-func")) != param.end())
      m_lr_func_name = m->second;
    // accommodate typo in Germann, 2015: Sampling Phrase Tables for
    // the Moses SMT System (PBML):
    if ((m = param.find("lrfunc")) != param.end())
      m_lr_func_name = m->second;

    if ((m = param.find("extra")) != param.end())
      m_extra_data = m->second;

    if ((m = param.find("method")) != param.end())
      {
        if (m->second == "random")
          m_sampling_method = random_sampling;
        else if (m->second == "ranked")
          m_sampling_method = ranked_sampling;
        else if (m->second == "ranked2")
          m_sampling_method = ranked_sampling2;
        else if (m->second == "full")
          m_sampling_method = full_coverage;
        else UTIL_THROW2("unrecognized specification 'method='" << m->second
                         << "' in line:\n" << line);
      }
    
    dflt = pair<string,string>("tuneable","true");
    m_tuneable = Scan<bool>(param.insert(dflt).first->second.c_str());

    dflt = pair<string,string>("feature-sets","standard");
    m_feature_set_names = Tokenize(param.insert(dflt).first->second.c_str(), ",");
    m = param.find("name");
    if (m != param.end()) m_name = m->second;

    // Optional coordinates for training corpus
    // Takes form coord=name1:file1.gz,name2:file2.gz,...
    // Names should match with XML input (coord tag)
    param.insert(pair<string,string>("coord","0"));
    if(param["coord"] != "0")
      {
        m_track_coord = true;
        vector<string> coord_instances = Tokenize(param["coord"], ",");
        BOOST_FOREACH(std::string instance, coord_instances)
          {
            vector<string> toks = Moses::Tokenize(instance, ":");
            string space = toks[0];
            string file = toks[1];
            // Register that this model uses the given space
            m_coord_spaces.push_back(StaticData::InstanceNonConst().MapCoordSpace(space));
            // Load sid coordinates from file
            m_sid_coord_list.push_back(vector<SPTR<vector<float> > >());
            vector<SPTR<vector<float> > >& sid_coord = m_sid_coord_list[m_sid_coord_list.size() - 1];
            //TODO: support extra data for btdyn, here? extra?
            sid_coord.reserve(btfix->T1->size());
            string line;
            cerr << "Loading coordinate lines for space \"" << space << "\" from " << file << endl;
            iostreams::filtering_istream in;
            ugdiss::open_input_stream(file, in);
            while(getline(in, line))
              {
                SPTR<vector<float> > coord(new vector<float>);
                Scan<float>(*coord, Tokenize(line));
                sid_coord.push_back(coord);
              }
            cerr << "Loaded " << sid_coord.size() << " lines" << endl;
          }
      }

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
    known_parameters.push_back("coord");
    known_parameters.push_back("cumb");
    known_parameters.push_back("extra");
    known_parameters.push_back("feature-sets");
    known_parameters.push_back("input-factor");
    known_parameters.push_back("lenrat");
    known_parameters.push_back("lexalpha");
    // known_parameters.push_back("limit"); // replaced by "table-limit"
    known_parameters.push_back("logcnt");
    known_parameters.push_back("lr-func"); // associated lexical reordering function
    known_parameters.push_back("lrfunc");  // associated lexical reordering function
    known_parameters.push_back("method");
    known_parameters.push_back("name");
    known_parameters.push_back("num-features");
    known_parameters.push_back("output-factor");
    known_parameters.push_back("path");
    known_parameters.push_back("pbwd");
    known_parameters.push_back("pfwd");
    known_parameters.push_back("prov");
    known_parameters.push_back("rare");
    known_parameters.push_back("sample");
    known_parameters.push_back("min-sample");
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
    m_bias = btfix->loadSentenceBias(fname);
  }

  void
  Mmsapt::
  load_extra_data(string bname, bool locking = true)
  {
    using namespace boost;
    using namespace ugdiss;
    // TO DO: ADD CHECKS FOR ROBUSTNESS
    // - file existence?
    // - same number of lines?
    // - sane word alignment?
    vector<string> text1,text2,symal;
    string line;
    boost::iostreams::filtering_istream in1,in2,ina;

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
  check_ff(string const ffname, vector<SPTR<pscorer> >* registry)
  {
    string const& spec = param[ffname];
    if (spec == "" || spec == "0") return;
    if (registry)
      {
        SPTR<fftype> ff(new fftype(spec));
        register_ff(ff, *registry);
      }
    else if (spec[spec.size()-1] == '+') // corpus specific
      {
        SPTR<fftype> ff(new fftype(spec));
        register_ff(ff, m_active_ff_fix);
        ff.reset(new fftype(spec));
        register_ff(ff, m_active_ff_dyn);
      }
    else
      {
        SPTR<fftype> ff(new fftype(spec));
        register_ff(ff, m_active_ff_common);
      }
  }

  template<typename fftype>
  void
  Mmsapt::
  check_ff(string const ffname, float const xtra,
           vector<SPTR<pscorer> >* registry)
  {
    string const& spec = param[ffname];
    if (spec == "" || spec == "0") return;
    if (registry)
      {
        SPTR<fftype> ff(new fftype(xtra,spec));
        register_ff(ff, *registry);
      }
    else if (spec[spec.size()-1] == '+') // corpus specific
      {
        SPTR<fftype> ff(new fftype(xtra,spec));
        register_ff(ff, m_active_ff_fix);
        ff.reset(new fftype(xtra,spec));
        register_ff(ff, m_active_ff_dyn);
      }
    else
      {
        SPTR<fftype> ff(new fftype(xtra,spec));
        register_ff(ff, m_active_ff_common);
      }
  }
  
  void
  Mmsapt::
  Load(AllOptions::ptr const& opts)
  {
    Load(opts, true);
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
            SPTR<PScoreLex1<Token> >
              ff(new PScoreLex1<Token>(param["lex_alpha"],lexfile));
            register_ff(ff,m_active_ff_common);
            
            // these are always computed on pooled data
            check_ff<PScoreRareness<Token> > ("rare", &m_active_ff_common);
            check_ff<PScoreUnaligned<Token> >("unal", &m_active_ff_common);
            check_ff<PScoreCoherence<Token> >("coh",  &m_active_ff_common);
            check_ff<PScoreCumBias<Token> >("cumb",  &m_active_ff_common);
            check_ff<PScoreLengthRatio<Token> > ("lenrat", &m_active_ff_common);
            
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
            SPTR<PScorePC<Token> > ffpcnt(new PScorePC<Token>("pcnt"));
            register_ff(ffpcnt,m_active_ff_common);
            SPTR<PScoreWC<Token> > ffwcnt(new PScoreWC<Token>("wcnt"));
            register_ff(ffwcnt,m_active_ff_common);
          }
      }
    // cerr << "Features: " << Join("|",m_feature_names) << endl;
    this->m_numScoreComponents = this->m_feature_names.size();
    this->m_numTuneableComponents  = this->m_numScoreComponents;
  }

  void
  Mmsapt::
  Load(AllOptions::ptr const& opts, bool with_checks)
  {
    m_options = opts;
    boost::unique_lock<boost::shared_mutex> lock(m_lock);
    // load feature functions (i.e., load underlying data bases, if any)
    BOOST_FOREACH(SPTR<pscorer>& ff, m_active_ff_fix) ff->load();
    BOOST_FOREACH(SPTR<pscorer>& ff, m_active_ff_dyn) ff->load();
    BOOST_FOREACH(SPTR<pscorer>& ff, m_active_ff_common) ff->load();
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

    m_thread_pool.reset(new ug::ThreadPool(max(m_workers,size_t(1))));

    // Load corpora. For the time being, we can have one memory-mapped static
    // corpus and one in-memory dynamic corpus

    btfix->m_num_workers = this->m_workers;
    btfix->open(m_bname, L1, L2);
    btfix->setDefaultSampleSize(m_default_sample_size);

    btdyn.reset(new imbitext(btfix->V1, btfix->V2, m_default_sample_size, m_workers));
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
  mkTPhrase(ttasksptr const& ttask,
            Phrase const& src,
            PhrasePair<Token>* fix,
            PhrasePair<Token>* dyn,
            SPTR<Bitext<Token> > const& dynbt) const
  {
    UTIL_THROW_IF2(!fix && !dyn, HERE <<
                   ": Can't create target phrase from nothing.");
    vector<float> fvals(this->m_numScoreComponents);
    PhrasePair<Token> pool = fix ? *fix : *dyn;
    if (fix)
      {
        BOOST_FOREACH(SPTR<pscorer> const& ff, m_active_ff_fix)
          (*ff)(*btfix, *fix, &fvals);
      }
    if (dyn)
      {
        BOOST_FOREACH(SPTR<pscorer> const& ff, m_active_ff_dyn)
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
        BOOST_FOREACH(SPTR<pscorer> const& ff, m_active_ff_dyn)
          (*ff)(*dynbt, ff->allowPooling() ? pool : zilch, &fvals);
      }
    else if (dyn)
      {
        PhrasePair<Token> zilch; zilch.init();
        TSA<Token>::tree_iterator m(btfix->I2.get(), dyn->start2, dyn->len2);
        if (m.size() == dyn->len2)
          zilch.raw2 = m.approxOccurrenceCount();
        pool += zilch;
        BOOST_FOREACH(SPTR<pscorer> const& ff, m_active_ff_fix)
          (*ff)(*dynbt, ff->allowPooling() ? pool : zilch, &fvals);
      }
    if (fix)
      {
        BOOST_FOREACH(SPTR<pscorer> const& ff, m_active_ff_common)
          (*ff)(*btfix, pool, &fvals);
      }
    else
      {
        BOOST_FOREACH(SPTR<pscorer> const& ff, m_active_ff_common)
          (*ff)(*dynbt, pool, &fvals);
      }

    TargetPhrase* tp = new TargetPhrase(const_cast<ttasksptr&>(ttask), this);
    Token const* x = fix ? fix->start2 : dyn->start2;
    uint32_t len = fix ? fix->len2 : dyn->len2;
    for (uint32_t k = 0; k < len; ++k, x = x->next())
      {
        StringPiece wrd = (*(btfix->V2))[x->id()];
        Word w; 
        w.CreateFromString(Output, m_ofactor, wrd, false);
        tp->AddWord(w);
      }
    tp->SetAlignTerm(pool.aln);
    tp->GetScoreBreakdown().Assign(this, fvals);
    // Evaluate with all features that can be computed using available factors
    tp->EvaluateInIsolation(src, m_featuresToApply);

#ifndef NO_MOSES
    if (m_lr_func)
      {
        LRModel::ModelType mdl = m_lr_func->GetModel().GetModelType();
        LRModel::Direction dir = m_lr_func->GetModel().GetDirection();
        SPTR<Scores> scores(new Scores());
        pool.fill_lr_vec(dir, mdl, *scores);
        tp->SetExtraScores(m_lr_func, scores);
      }
#endif

    // Track coordinates if requested
    if (m_track_coord)
    {
      BOOST_FOREACH(uint32_t const sid, *pool.sids)
        {
          for(size_t i = 0; i < m_coord_spaces.size(); ++i)
            {
              tp->PushCoord(m_coord_spaces[i], m_sid_coord_list[i][sid]);
            }
        }
      /*
      cerr << btfix->toString(pool.p1, 0) << " ::: " << btfix->toString(pool.p2, 1);
      BOOST_FOREACH(size_t id, m_coord_spaces)
        {
          cerr << " [" << id << "]";
          vector<vector<float> const*> const* coordList = tp->GetCoordList(id);
          BOOST_FOREACH(vector<float> const* coord, *coordList)
            cerr << " : " << Join(" ", *coord);
        }
      cerr << endl;
      */
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
        TargetPhraseCollection::shared_ptr targetPhrases
          = this->GetTargetPhraseCollectionLEGACY(ttask,phrase);
        inputPath.SetTargetPhrases(*this, targetPhrases, NULL);
      }
  }
  
  // TargetPhraseCollection::shared_ptr
  // Mmsapt::
  // GetTargetPhraseCollectionLEGACY(const Phrase& src) const
  // {
  //   UTIL_THROW2("Don't call me without the translation task.");
  // }

  // This is not the most efficient way of phrase lookup!
  TargetPhraseCollection::shared_ptr
  Mmsapt::
  GetTargetPhraseCollectionLEGACY(ttasksptr const& ttask, const Phrase& src) const
  {
    SPTR<TPCollWrapper> ret;
    // boost::unique_lock<boost::shared_mutex> xlock(m_lock);

    // map from Moses Phrase to internal id sequence
    vector<id_type> sphrase;
    fillIdSeq(src, m_ifactor, *(btfix->V1), sphrase);
    if (sphrase.size() == 0) return ret;
    
    // Reserve a local copy of the dynamic bitext in its current form. /btdyn/
    // is set to a new copy of the dynamic bitext every time a sentence pair
    // is added. /dyn/ keeps the old bitext around as long as we need it.
    SPTR<imBitext<Token> > dyn;
    { // braces are needed for scoping mutex lock guard!
      boost::unique_lock<boost::shared_mutex> guard(m_lock);
      assert(btdyn);
      dyn = btdyn;
    }
    assert(dyn);

    // lookup phrases in both bitexts
    TSA<Token>::tree_iterator mfix(btfix->I1.get(), &sphrase[0], sphrase.size());
    TSA<Token>::tree_iterator mdyn(dyn->I1.get());
    if (dyn->I1.get()) // we have a dynamic bitext
      for (size_t i = 0; mdyn.size() == i && i < sphrase.size(); ++i)
        mdyn.extend(sphrase[i]);

    if (mdyn.size() != sphrase.size() && mfix.size() != sphrase.size())
      return ret; // phrase not found in either bitext

    // do we have cached results for this phrase?
    uint64_t phrasekey = (mfix.size() == sphrase.size()
                          ? (mfix.getPid()<<1) 
                          : (mdyn.getPid()<<1)+1);

    // get context-specific cache of items previously looked up
    SPTR<ContextScope> const& scope = ttask->GetScope();
    SPTR<TPCollCache> cache = scope->get<TPCollCache>(cache_key);
    if (!cache) cache = m_cache; // no context-specific cache, use global one

    ret = cache->get(phrasekey, dyn->revision());
    // TO DO: we should revise the revision mechanism: we take the
    // length of the dynamic bitext (in sentences) at the time the PT
    // entry was stored as the time stamp. For each word in the
    // vocabulary, we also store its most recent occurrence in the
    // bitext. Only if the timestamp of each word in the phrase is
    // newer than the timestamp of the phrase itself we must update
    // the entry.

    // std::cerr << "Phrasekey is " << ret->key << " at " << HERE << std::endl;
    // std::cerr << ret << " with " << ret->refCount << " references at " 
    // << HERE << std::endl;
    boost::upgrade_lock<boost::shared_mutex> rlock(ret->lock);
    if (ret->GetSize()) return ret;

    // new TPC (not found or old one was not up to date)
    boost::upgrade_to_unique_lock<boost::shared_mutex> wlock(rlock);
    // maybe another thread did the work while we waited for the lock ?
    if (ret->GetSize()) return ret;

    // OK: pt entry NOT found or NOT up to date
    // lookup and expansion could be done in parallel threads,
    // but ppdyn is probably small anyway
    // TO DO: have Bitexts return lists of PhrasePairs instead of pstats
    // no need to expand pstats at every single lookup again, especially
    // for btfix.
    SPTR<pstats> sfix,sdyn;

    if (mfix.size() == sphrase.size()) 
      {
        SPTR<ContextForQuery> context = scope->get<ContextForQuery>(btfix.get());
        SPTR<pstats> const* foo = context->cache1->get(mfix.getPid());
        if (foo) { sfix = *foo; sfix->wait(); }
        else 
          {
            BitextSampler<Token> s(btfix, mfix, context->bias, 
                                   m_min_sample_size, 
                                   m_default_sample_size, 
                                   m_sampling_method,
                                   m_track_coord);
            s();
            sfix = s.stats();
          }
      }

    if (mdyn.size() == sphrase.size()) 
      sdyn = dyn->lookup(ttask, mdyn);

    vector<PhrasePair<Token> > ppfix,ppdyn;
    PhrasePair<Token>::SortByTargetIdSeq sort_by_tgt_id;
    if (sfix)
      {
        expand(mfix, *btfix, *sfix, ppfix, m_bias_log);
        sort(ppfix.begin(), ppfix.end(),sort_by_tgt_id);
      }
    if (sdyn)
      {
        expand(mdyn, *dyn, *sdyn, ppdyn, m_bias_log);
        sort(ppdyn.begin(), ppdyn.end(),sort_by_tgt_id);
      }

    // now we have two lists of Phrase Pairs, let's merge them
    PhrasePair<Token>::SortByTargetIdSeq sorter;
    size_t i = 0; size_t k = 0;
    while (i < ppfix.size() && k < ppdyn.size())
      {
        int cmp = sorter.cmp(ppfix[i], ppdyn[k]);
        if      (cmp  < 0) ret->Add(mkTPhrase(ttask,src,&ppfix[i++],NULL,dyn));
        else if (cmp == 0) ret->Add(mkTPhrase(ttask,src,&ppfix[i++],&ppdyn[k++],dyn));
        else               ret->Add(mkTPhrase(ttask,src,NULL,&ppdyn[k++],dyn));
      }
    while (i < ppfix.size()) ret->Add(mkTPhrase(ttask,src,&ppfix[i++],NULL,dyn));
    while (k < ppdyn.size()) ret->Add(mkTPhrase(ttask,src,NULL,&ppdyn[k++],dyn));

    // Pruning should not be done here but outside!
    if (m_tableLimit) ret->Prune(true, m_tableLimit);
    else ret->Prune(true,ret->GetSize());

#if 1
    if (m_bias_log && m_lr_func && m_bias_loglevel > 3)
      {
        PhrasePair<Token>::SortDescendingByJointCount sorter;
        sort(ppfix.begin(), ppfix.end(),sorter);
        BOOST_FOREACH(PhrasePair<Token> const& pp, ppfix)
          {
            // if (&pp != &ppfix.front() && pp.joint <= 1) break;
            pp.print(*m_bias_log,*btfix->V1, *btfix->V2, m_lr_func->GetModel());
          }
      }
#endif
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
                          size_t )
  {
    throw "CreateRuleLookupManager is currently not supported in Mmsapt!";
  }

  void
  Mmsapt::
  setup_bias(ttasksptr const& ttask)
  {
    SPTR<ContextScope> const& scope = ttask->GetScope();
    SPTR<ContextForQuery> context;
    context = scope->get<ContextForQuery>(btfix.get(), true);
    if (context->bias) return; 
    
    // bias weights specified with the session?
    SPTR<std::map<std::string, float> const> w;
    w = ttask->GetScope()->GetContextWeights();
    if (w && !w->empty()) 
      {
        if (m_bias_log) 
          *m_bias_log << "BIAS WEIGHTS GIVEN WITH INPUT at " << HERE << endl;
        context->bias = btfix->SetupDocumentBias(*w, m_bias_log);
      }
    else if (m_bias_server.size() && ttask->GetContextWindow())
      {
        // std::cerr << "via server at " << HERE << std::endl;
        string context_words;
        BOOST_FOREACH(string const& line, *ttask->GetContextWindow())
          {
            if (context_words.size()) context_words += " ";
            context_words += line;
          }
        if (context_words.size())
          {
            if (m_bias_log)
              *m_bias_log << "GETTING BIAS FROM SERVER at " << HERE << endl
                          << "BIAS LOOKUP CONTEXT: " << context_words << endl;
            context->bias
              = btfix->SetupDocumentBias(m_bias_server,context_words,m_bias_log);
            //Reset the bias in the ttaskptr so that other functions
            //so that other functions can utilize the biases;
            ttask->GetScope()->SetContextWeights(context->bias->getBiasMap());
          }
      } 
    if (context->bias)
      {
        context->bias_log = m_bias_log;
        context->bias->loglevel = m_bias_loglevel;
      }
  }
  
  void
  Mmsapt::
  InitializeForInput(ttasksptr const& ttask)
  {
    boost::unique_lock<boost::shared_mutex> mylock(m_lock);
    
    SPTR<ContextScope> const& scope = ttask->GetScope();
    SPTR<TPCollCache> localcache = scope->get<TPCollCache>(cache_key);
    SPTR<ContextForQuery> context = scope->get<ContextForQuery>(btfix.get(), true);
    boost::unique_lock<boost::shared_mutex> ctxlock(context->lock);

    // if (localcache) std::cerr << "have local cache " << std::endl;
    // std::cerr << "BOO at " << HERE << std::endl;
    if (!localcache)
      {
        // std::cerr << "no local cache at " << HERE << std::endl;
        setup_bias(ttask);
        if (context->bias) 
          {
            localcache.reset(new TPCollCache(m_cache_size));
          }
        else localcache = m_cache;
        scope->set<TPCollCache>(cache_key, localcache);
      }

    if (!context->cache1) context->cache1.reset(new pstats::cache_t);
    if (!context->cache2) context->cache2.reset(new pstats::cache_t);
    
#ifndef NO_MOSES
    if (m_lr_func_name.size() && m_lr_func == NULL)
      {
        FeatureFunction* lr = &FeatureFunction::FindFeatureFunction(m_lr_func_name);
        m_lr_func = dynamic_cast<LexicalReordering*>(lr);
        UTIL_THROW_IF2(lr == NULL, "FF " << m_lr_func_name
                       << " does not seem to be a lexical reordering function!");
        // todo: verify that lr_func implements a hierarchical reordering model
      }
#endif
  }

  bool
  Mmsapt::
  PrefixExists(ttasksptr const& ttask, Moses::Phrase const& phrase) const
  {
    if (phrase.GetSize() == 0) return false;
    SPTR<ContextScope> const& scope = ttask->GetScope();

    vector<id_type> myphrase; 
    fillIdSeq(phrase, m_ifactor, *btfix->V1, myphrase);

    TSA<Token>::tree_iterator mfix(btfix->I1.get(),&myphrase[0],myphrase.size());
    if (mfix.size() == myphrase.size())
      {
        SPTR<ContextForQuery> context = scope->get<ContextForQuery>(btfix.get(), true);
        uint64_t pid = mfix.getPid();
        if (!context->cache1->get(pid))
          {
            BitextSampler<Token> s(btfix, mfix, context->bias, 
                                   m_min_sample_size, m_default_sample_size, 
                                   m_sampling_method, m_track_coord);
            if (*context->cache1->get(pid, s.stats()) == s.stats())
              m_thread_pool->add(s);
          }
        // btfix->prep(ttask, mfix);
        // cerr << phrase << " " << mfix.approxOccurrenceCount() << endl;
        return true;
      }

    SPTR<imBitext<Token> > dyn;
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
        if (mdyn.size() == myphrase.size()) dyn->prep(ttask, mdyn, m_track_coord);
      }
    return mdyn.size() == myphrase.size();
  }

#if 0
  void
  Mmsapt
  ::Release(ttasksptr const& ttask, TargetPhraseCollection::shared_ptr*& tpc) const
  {
    if (!tpc) 
      {
        // std::cerr << "NULL pointer at " << HERE << std::endl;
        return; 
      }
    SPTR<TPCollCache> cache = ttask->GetScope()->get<TPCollCache>(cache_key);

    TPCollWrapper const* foo = static_cast<TPCollWrapper const*>(tpc);

    // std::cerr << "\nReleasing " << foo->key << "\n" << std::endl;

    if (cache) cache->release(static_cast<TPCollWrapper const*>(tpc));
    tpc = NULL;
  }
#endif

  bool Mmsapt
  ::ProvidesPrefixCheck() const { return true; }

  string const& Mmsapt
  ::GetName() const { return m_name; }

  // SPTR<DocumentBias>
  // Mmsapt
  // ::setupDocumentBias(map<string,float> const& bias) const
  // {
  //   return btfix->SetupDocumentBias(bias);
  // }

  vector<float>
  Mmsapt
  ::DefaultWeights() const
  { return vector<float>(this->GetNumScoreComponents(), 1.); }

}
