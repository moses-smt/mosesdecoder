// -*- c++ -*-
// Sampling phrase table implementation based on memory-mapped suffix arrays.
// Design and code by Ulrich Germann.
#pragma once

#include <time.h>
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
#include "moses/TranslationModel/UG/mm/ug_phrasepair.h"
#include "moses/TranslationModel/UG/mm/ug_lexical_phrase_scorer2.h"

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

using namespace std;
namespace Moses
{
  using namespace bitext;
  class Mmsapt 
#ifndef NO_MOSES
    : public PhraseDictionary
#endif
  {
    friend class Alignment;
    map<string,string> param;
    vector<float> bias;
    string m_name;
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
    string bname,extra_data,bias_file;
    string L1;
    string L2;
    float  m_lbop_conf; // confidence level for lbop smoothing
    float  m_lex_alpha; // alpha paramter (j+a)/(m+a) for lexical smoothing
    // alpha parameter for lexical smoothing (joint+alpha)/(marg + alpha)
    // must be > 0 if dynamic 
    size_t m_default_sample_size;
    size_t m_workers;  // number of worker threads for sampling the bitexts
    vector<string> m_feature_set_names; // one or more of: standard, datasource
 
    // // deprecated!
    // char m_pfwd_denom; // denominator for computation of fwd phrase score:
    // // 'r' - divide by raw count
    // // 's' - divide by sample count
    // // 'g' - devide by number of "good" (i.e. coherent) samples 
    // // size_t num_features;

    size_t input_factor;
    size_t output_factor; // we can actually return entire Tokens!

    // bool withLogCountFeatures; // add logs of counts as features?
    // bool withCoherence; 
    // string m_pfwd_features; // which pfwd functions to use
    // string m_pbwd_features; // which pbwd functions to use

    // for display for human inspection (ttable dumps):
    vector<string> m_feature_names; // names of features activated
    vector<bool> m_is_logval;  // keeps track of which features are log valued 
    vector<bool> m_is_integer; // keeps track of which features are integer valued 

    vector<sptr<pscorer > > m_active_ff_fix; // activated feature functions (fix)
    vector<sptr<pscorer > > m_active_ff_dyn; // activated feature functions (dyn)
    vector<sptr<pscorer > > m_active_ff_common; // activated feature functions (dyn)

    void
    register_ff(sptr<pscorer> const& ff, vector<sptr<pscorer> > & registry);

    template<typename fftype>
    void 
    check_ff(string const ffname,vector<sptr<pscorer> >* registry = NULL);
    // add feature function if specified 
    
    template<typename fftype>
    void 
    check_ff(string const ffname, float const xtra, vector<sptr<pscorer> >* registry = NULL);
    // add feature function if specified

    void
    add_corpus_specific_features(vector<sptr<pscorer > >& ffvec);
    
    // built-in feature functions
    // PScorePfwd<Token> calc_pfwd_fix, calc_pfwd_dyn;
    // PScorePbwd<Token> calc_pbwd_fix, calc_pbwd_dyn;
    // PScoreLex<Token>  calc_lex; // this one I'd like to see as an external ff eventually
    // PScorePC<Token>   apply_pp; // apply phrase penalty 
    // PScoreLogCounts<Token>   add_logcounts_fix;
    // PScoreLogCounts<Token>   add_logcounts_dyn;
    void init(string const& line);
    mutable boost::mutex lock;
    bool withPbwd;
    bool poolCounts;
    vector<FactorType> ofactor;


  public:
    // typedef boost::unordered_map<uint64_t, sptr<TargetPhraseCollection> > tpcoll_cache_t;
    class TargetPhraseCollectionWrapper 
      : public TargetPhraseCollection
    {
    public:
      size_t   const revision; // time stamp from dynamic bitext
      ::uint64_t const      key; // phrase key
      uint32_t       refCount; // reference count
#if defined(timespec)
      timespec         tstamp; // last use
#else
      timeval          tstamp; // last use
#endif
      int                 idx; // position in history heap
      TargetPhraseCollectionWrapper(size_t r, ::uint64_t const k);
      ~TargetPhraseCollectionWrapper();
    };

  private:

    void read_config_file(string fname, map<string,string>& param);

    TargetPhraseCollectionWrapper*
    encache(TargetPhraseCollectionWrapper* const ptr) const;

    void
    decache(TargetPhraseCollectionWrapper* ptr) const;

    typedef map<typename ::uint64_t, TargetPhraseCollectionWrapper*> tpc_cache_t;
    mutable tpc_cache_t m_cache;
    mutable vector<TargetPhraseCollectionWrapper*> m_history;
    // phrase table feature weights for alignment:
    vector<float> feature_weights; 

    vector<vector<id_type> > wlex21; 
    // word translation lexicon (without counts, get these from calc_lex.COOC)
    typedef mm2dTable<id_type,id_type,uint32_t,uint32_t> mm2dtable_t;
    mm2dtable_t COOCraw;

    TargetPhrase* 
    mkTPhrase(Phrase const& src, 
	      Moses::bitext::PhrasePair<Token>* fix, 
	      Moses::bitext::PhrasePair<Token>* dyn, 
	      sptr<Bitext<Token> > const& dynbt) const;

    // template<typename Token>
    // void 
    // expand(typename Bitext<Token>::iter const& m, Bitext<Token> const& bt, 
    // 	   pstats const& pstats, vector<PhrasePair<Token> >& dest);
    
#if 0
    TargetPhrase* 
    mkTPhrase
    (Phrase        const& src, 
     Bitext<Token> const& bt, 
     Moses::bitext::PhrasePair const& pp
     ) const;
#endif
    void
    process_pstats
    (Phrase   const& src,
     ::uint64_t const  pid1, 
     pstats   const& stats, 
     Bitext<Token> const & bt, 
     TargetPhraseCollection* tpcoll
     ) const;

    bool
    pool_pstats
    (Phrase   const& src,
     ::uint64_t const  pid1a, 
     pstats        * statsa, 
     Bitext<Token> const & bta,
     ::uint64_t const  pid1b, 
     pstats   const* statsb, 
     Bitext<Token> const & btb,
     TargetPhraseCollection* tpcoll
     ) const;
     
    bool
    combine_pstats
    (Phrase   const& src,
     ::uint64_t const  pid1a, 
     pstats   * statsa, 
     Bitext<Token> const & bta,
     ::uint64_t const  pid1b, 
     pstats   const* statsb, 
     Bitext<Token> const & btb,
     TargetPhraseCollection* tpcoll
     ) const;

    void
    load_extra_data(string bname, bool locking);

    void
    load_bias(string bname);

    mutable size_t m_tpc_ctr;
  public:
    // Mmsapt(string const& description, string const& line);
    Mmsapt(string const& line);

    void
    Load();

    void
    Load(bool with_checks);
    
    // returns the prior table limit
    size_t SetTableLimit(size_t limit);

    string const&
    GetName() const;

#ifndef NO_MOSES
    TargetPhraseCollection const* 
    GetTargetPhraseCollectionLEGACY(const Phrase& src) const;
    //! Create a sentence-specific manager for SCFG rule lookup.
    ChartRuleLookupManager*
    CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase &);

    ChartRuleLookupManager*
    CreateRuleLookupManager
    (const ChartParser &, const ChartCellCollectionBase &, std::size_t);
#endif

    void add(string const& s1, string const& s2, string const& a);

    // align two new sentences
    sptr<vector<int> >
    align(string const& src, string const& trg) const;

    void setWeights(vector<float> const& w);

    void 
    CleanUpAfterSentenceProcessing(const InputType& source);

    void 
    InitializeForInput(InputType const& source);

    void 
    Release(TargetPhraseCollection const* tpc) const;

    bool 
    ProvidesPrefixCheck() const;
    
    /// return true if prefix /phrase/ exists
    bool
    PrefixExists(Phrase const& phrase, vector<float> const* const bias) const;

    bool
    PrefixExists(Phrase const& phrase) const;

    vector<string> const&
    GetFeatureNames() const;
    
    // void
    // ScorePPfix(bitext::PhrasePair& pp) const;

    bool
    isLogVal(int i) const;
    
    bool
    isInteger(int i) const;

  private:
  };
} // end namespace

