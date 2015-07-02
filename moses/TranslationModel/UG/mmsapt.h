// -*- c++ -*-
// Sampling phrase table implementation based on memory-mapped suffix arrays.
// Design and code by Ulrich Germann.
#pragma once

#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>

#include "moses/TypeDef.h"
#include "moses/TranslationModel/UG/generic/sorting/VectorIndexSorter.h"
#include "moses/TranslationModel/UG/generic/sampling/Sampling.h"
#include "moses/TranslationModel/UG/generic/file_io/ug_stream.h"

#include "moses/TranslationModel/UG/mm/ug_mm_ttrack.h"
#include "moses/TranslationModel/UG/mm/ug_mm_tsa.h"
#include "moses/TranslationModel/UG/mm/tpt_tokenindex.h"
#include "moses/TranslationModel/UG/mm/ug_corpus_token.h"
#include "moses/TranslationModel/UG/mm/ug_typedefs.h"
#include "moses/TranslationModel/UG/mm/tpt_pickler.h"
#include "moses/TranslationModel/UG/mm/ug_bitext.h"
#include "moses/TranslationModel/UG/mm/ug_lexical_phrase_scorer2.h"

#include "moses/TranslationModel/UG/TargetPhraseCollectionCache.h"

#include "moses/FF/LexicalReordering/LexicalReordering.h"

#include "moses/InputFileStream.h"
#include "moses/FactorTypeSet.h"
#include "moses/TargetPhrase.h"
#include <boost/dynamic_bitset.hpp>
#include "moses/TargetPhraseCollection.h"
#include "util/usage.hh"
#include <map>

#include "moses/TranslationModel/PhraseDictionary.h"
#include "sapt_phrase_scorers.h"

// TO DO:
// - make lexical phrase scorer take addition to the "dynamic overlay" into account
// - switch to pool of sapts, where each sapt has its own provenance feature
//   RESEARCH QUESTION: is this more effective than having multiple phrase tables,
//   each with its own set of features?

namespace Moses
{
  using namespace bitext;
  class Mmsapt
#ifndef NO_MOSES
    : public PhraseDictionary
#endif
  {
    // using namespace std;
    class TPCOllCache;
    friend class Alignment;
    std::map<std::string,std::string> param;
    std::string m_name;
  public:
    typedef L2R_Token<SimpleWordId> Token;
    typedef mmBitext<Token> mmbitext;
    typedef imBitext<Token> imbitext;
    typedef Bitext<Token>     bitext;
    typedef TSA<Token>           tsa;
    typedef PhraseScorer<Token> pscorer;
  private:
    // vector<sptr<bitext> > shards;
    mmbitext btfix;
    sptr<imbitext> btdyn;
    std::string m_bname, m_extra_data, m_bias_file,m_bias_server;
    std::string L1;
    std::string L2;
    float  m_lbop_conf; // confidence level for lbop smoothing
    float  m_lex_alpha; // alpha paramter (j+a)/(m+a) for lexical smoothing
    // alpha parameter for lexical smoothing (joint+alpha)/(marg + alpha)
    // must be > 0 if dynamic
    size_t m_default_sample_size;
    size_t m_workers;  // number of worker threads for sampling the bitexts
    std::vector<std::string> m_feature_set_names; // one or more of: standard, datasource
    std::string m_bias_logfile;
    boost::scoped_ptr<ofstream> m_bias_logger; // for logging to a file
    ostream* m_bias_log;
    int m_bias_loglevel;
    LexicalReordering* m_lr_func; // associated lexical reordering function
    string m_lr_func_name; // name of associated lexical reordering function
  public:
    void* const  cache_key;   // for getting cache from ttask
    void* const  context_key; // for context scope from ttask
  private:
    boost::shared_ptr<SamplingBias> m_bias; // for global default bias
    boost::shared_ptr<TPCollCache> m_cache; // for global default bias
    size_t m_cache_size;  //
    size_t input_factor;  //
    size_t output_factor; // we can actually return entire Tokens!

    // for display for human inspection (ttable dumps):
    std::vector<std::string> m_feature_names; // names of features activated
    std::vector<bool> m_is_logval;  // keeps track of which features are log valued
    std::vector<bool> m_is_integer; // keeps track of which features are integer valued

    std::vector<sptr<pscorer > > m_active_ff_fix; // activated feature functions (fix)
    std::vector<sptr<pscorer > > m_active_ff_dyn; // activated feature functions (dyn)
    std::vector<sptr<pscorer > > m_active_ff_common;
    // activated feature functions (dyn)

    void
    register_ff(sptr<pscorer> const& ff, std::vector<sptr<pscorer> > & registry);

    template<typename fftype>
    void
    check_ff(std::string const ffname,std::vector<sptr<pscorer> >* registry = NULL);
    // add feature function if specified

    template<typename fftype>
    void
    check_ff(std::string const ffname, float const xtra,
	     std::vector<sptr<pscorer> >* registry = NULL);
    // add feature function if specified

    void
    add_corpus_specific_features(std::vector<sptr<pscorer > >& ffvec);

    // built-in feature functions
    // PScorePfwd<Token> calc_pfwd_fix, calc_pfwd_dyn;
    // PScorePbwd<Token> calc_pbwd_fix, calc_pbwd_dyn;
    // PScoreLex<Token>  calc_lex;
    // this one I'd like to see as an external ff eventually
    // PScorePC<Token>   apply_pp; // apply phrase penalty
    // PScoreLogCounts<Token>   add_logcounts_fix;
    // PScoreLogCounts<Token>   add_logcounts_dyn;
    void init(std::string const& line);
    mutable boost::shared_mutex m_lock;
    // mutable boost::shared_mutex m_cache_lock;
    // for more complex operations on the cache
    bool withPbwd;
    bool poolCounts;
    std::vector<FactorType> ofactor;

    void setup_local_feature_functions();

  private:

    void read_config_file(std::string fname, std::map<std::string,std::string>& param);

    // phrase table feature weights for alignment:
    std::vector<float> feature_weights;

    std::vector<std::vector<id_type> > wlex21;
    // word translation lexicon (without counts, get these from calc_lex.COOC)
    typedef ugdiss::mm2dTable<id_type,id_type,uint32_t,uint32_t> mm2dtable_t;
    mm2dtable_t COOCraw;

    TargetPhrase*
    mkTPhrase(ttasksptr const& ttask,
              Phrase const& src,
	      Moses::bitext::PhrasePair<Token>* fix,
	      Moses::bitext::PhrasePair<Token>* dyn,
	      sptr<Bitext<Token> > const& dynbt) const;

    void
    process_pstats
    (Phrase   const& src,
     uint64_t const  pid1,
     pstats   const& stats,
     Bitext<Token> const & bt,
     TargetPhraseCollection* tpcoll
     ) const;

    bool
    pool_pstats
    (Phrase   const& src,
     uint64_t const  pid1a, pstats * statsa, Bitext<Token> const & bta,
     uint64_t const  pid1b, pstats const* statsb, Bitext<Token> const & btb,
     TargetPhraseCollection* tpcoll) const;

    bool
    combine_pstats
    (Phrase   const& src,
     uint64_t const  pid1a, pstats* statsa, Bitext<Token> const & bta,
     uint64_t const  pid1b, pstats const* statsb, Bitext<Token> const & btb,
     TargetPhraseCollection* tpcoll) const;

    void load_extra_data(std::string bname, bool locking);
    void load_bias(std::string bname);

  public:
    // Mmsapt(std::string const& description, std::string const& line);
    Mmsapt(std::string const& line);

    void Load();
    void Load(bool with_checks);
    size_t SetTableLimit(size_t limit); // returns the prior table limit
    std::string const& GetName() const;

#ifndef NO_MOSES
    TargetPhraseCollection const*
    GetTargetPhraseCollectionLEGACY(ttasksptr const& ttask, const Phrase& src) const;

    TargetPhraseCollection const*
    GetTargetPhraseCollectionLEGACY(const Phrase& src) const;

    void
    GetTargetPhraseCollectionBatch(ttasksptr const& ttask,
				   const InputPathList &inputPathQueue) const;

    //! Create a sentence-specific manager for SCFG rule lookup.
    ChartRuleLookupManager*
    CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase &);

    ChartRuleLookupManager*
    CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase &,
			    std::size_t);
#endif

    void add(std::string const& s1, std::string const& s2, std::string const& a);
    // add a new sentence pair to the dynamic bitext

    void setWeights(std::vector<float> const& w);


    void Release(ttasksptr const& ttask, TargetPhraseCollection*& tpc) const;
    // some consumer lets me know that *tpc isn't needed any more


    bool ProvidesPrefixCheck() const; // return true if prefix /phrase/ check exists
    // bool PrefixExists(Phrase const& phrase, SamplingBias const* const bias) const;
    bool PrefixExists(ttasksptr const& ttask, Phrase const& phrase) const;

    bool isLogVal(int i) const;
    bool isInteger(int i) const;

    // task setup and takedown functions
    void InitializeForInput(ttasksptr const& ttask);
    // void CleanUpAfterSentenceProcessing(const InputType& source);
    void CleanUpAfterSentenceProcessing(ttasksptr const& ttask);

    // align two new sentences
    sptr<std::vector<int> >
    align(std::string const& src, std::string const& trg) const;

    std::vector<std::string> const&
    GetFeatureNames() const;

    sptr<DocumentBias>
    setupDocumentBias(std::map<std::string,float> const& bias) const;

    vector<float> DefaultWeights() const;
  };
} // end namespace

