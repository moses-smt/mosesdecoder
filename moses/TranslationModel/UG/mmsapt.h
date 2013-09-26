// -*- c++ -*-
// Sampling phrase table implementation based on memory-mapped suffix arrays.
// Design and code by Ulrich Germann.
#pragma once

#include <boost/thread.hpp>

#include "moses/generic/sorting/VectorIndexSorter.h"
#include "moses/generic/sampling/Sampling.h"
#include "moses/generic/file_io/ug_stream.h"

#include "moses/mm/ug_mm_ttrack.h"
#include "moses/mm/ug_mm_tsa.h"
#include "moses/mm/tpt_tokenindex.h"
#include "moses/mm/ug_corpus_token.h"
#include "moses/mm/ug_typedefs.h"
#include "moses/mm/tpt_pickler.h"
#include "moses/mm/ug_bitext.h"
#include "moses/mm/ug_lexical_phrase_scorer2.h"

#include "moses/InputFileStream.h"
#include "moses/FactorTypeSet.h"
#include "moses/TargetPhrase.h"
#include <boost/dynamic_bitset.hpp>
#include "moses/TargetPhraseCollection.h"
#include <map>

#include "PhraseDictionary.h"

using namespace std;
namespace Moses
{
  using namespace bitext;
  class Mmsapt : public PhraseDictionary
  {
    
    typedef L2R_Token<SimpleWordId> Token;
    typedef mmBitext<Token> mmbitext;
    mmbitext bt;
    
    // string description;
    string bname;
    string L1;
    string L2;
    float  lbop_parameter;
    size_t default_sample_size;
    // size_t num_features;
    size_t input_factor;
    size_t output_factor; // we can actually return entire Tokens!
    // built-in feature functions
    PScorePfwd<Token> calc_pfwd;
    PScorePbwd<Token> calc_pbwd;
    PScoreLex<Token>  calc_lex; // this one I'd like to see as an external ff eventually
    PScorePP<Token>   apply_pp; // apply phrase penalty 
    void init(string const& line);
    mutable boost::mutex lock;
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

  private:
  };
} // end namespace

