// -*- c++ -*-
// Sampling phrase table implementation based on memory-mapped suffix arrays.
// Design and code by Ulrich Germann.
#pragma once

#include <boost/thread.hpp>

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

#include "moses/InputFileStream.h"
#include "moses/FactorTypeSet.h"
#include "moses/TargetPhrase.h"
#include <boost/dynamic_bitset.hpp>
#include "moses/TargetPhraseCollection.h"
#include <map>

#include "moses/TranslationModel/PhraseDictionary.h"

// TO DO:
// - make lexical phrase scorer take addition to the "dynamic overlay" into account
// - switch to pool of sapts, where each sapt has its own provenance feature
//   RESEARCH QUESTION: is this more effective than having multiple phrase tables, 
//   each with its own set of features?

using namespace std;
namespace Moses
{
  using namespace bitext;
  class Mmsapt : public PhraseDictionary
  {
  public:    
    typedef L2R_Token<SimpleWordId> Token;
    typedef mmBitext<Token> mmbitext;
    typedef imBitext<Token> imbitext;
  private:
    mmbitext btfix;
    sptr<imbitext> btdyn;
    string bname;
    string L1;
    string L2;
    float  lbop_parameter;
    size_t default_sample_size;
    // size_t num_features;
    size_t input_factor;
    size_t output_factor; // we can actually return entire Tokens!
    // built-in feature functions
    PScorePfwd<Token> calc_pfwd_fix, calc_pfwd_dyn;
    PScorePbwd<Token> calc_pbwd_fix, calc_pbwd_dyn;
    PScoreLex<Token>  calc_lex; // this one I'd like to see as an external ff eventually
    PScorePP<Token>   apply_pp; // apply phrase penalty 
    void init(string const& line);
    mutable boost::mutex lock;
    bool poolCounts;
    vector<FactorType> ofactor;

    TargetPhrase* 
    createTargetPhrase
    (Phrase        const& src, 
     Bitext<Token> const& bt, 
     bitext::PhrasePair    const& pp
     ) const;

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
     uint64_t const  pid1a, 
     pstats        * statsa, 
     Bitext<Token> const & bta,
     uint64_t const  pid1b, 
     pstats   const* statsb, 
     Bitext<Token> const & btb,
     TargetPhraseCollection* tpcoll
     ) const;
     
    bool
    combine_pstats
    (Phrase   const& src,
     uint64_t const  pid1a, 
     pstats   * statsa, 
     Bitext<Token> const & bta,
     uint64_t const  pid1b, 
     pstats   const* statsb, 
     Bitext<Token> const & btb,
     TargetPhraseCollection* tpcoll
     ) const;

  public:
    Mmsapt(string const& description, string const& line);
    Mmsapt(string const& line);
    void
    Load();
    
    TargetPhraseCollection const* 
    GetTargetPhraseCollectionLEGACY(const Phrase& src) const;

    //! Create a sentence-specific manager for SCFG rule lookup.
    ChartRuleLookupManager*
    CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase &);

    void add(string const& s1, string const& s2, string const& a);

  private:
  };
} // end namespace

