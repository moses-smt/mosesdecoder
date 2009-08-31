/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <utility>

#ifdef LM_CACHE
#include <ext/hash_map>
#endif

//#include "DummyScoreProducers.h"
#include "SufficientStats.h"
#include "ScoreComponentCollection.h"


#ifdef LM_CACHE
namespace __gnu_cxx {
  template<> struct hash<std::vector<const Moses::Word*> > {
    inline size_t operator()(const std::vector<const Moses::Word*>& p) const {
      static const int primes[] = {8933, 8941, 8951, 8963, 8969, 8971, 8999, 9001, 9007, 9011};
      size_t h = 0;
      for (unsigned i = 0; i < p.size(); ++i)
        h += reinterpret_cast<size_t>(p[i]) * primes[i % 10];
      return h;
    }
  };
}
#endif

namespace Moses {
  class TranslationOption;
  class TranslationOptionCollection;
  class Hypothesis;
  class Factor;
  class WordsRange;
  class Word;
}

using namespace Moses;

namespace Josiah {

class Sample;
class GainFunction;
class GibbsOperator;  
struct TargetGap;  

#ifdef LM_CACHE
/* 
 * An MRU cache
**/
class LanguageModelCache {
  public:
    LanguageModelCache(LanguageModel* languageModel, int maxSize=100000) :
      m_languageModel(languageModel), m_maxSize(maxSize) {}
    float GetValue(const std::vector<const Word*>& ngram);
    
  private:
    LanguageModel* m_languageModel;
    int m_maxSize;
    //current entries
    typedef std::pair<std::vector<const Word*>,float> Entry;
    typedef std::list<Entry> EntryList;
    typedef EntryList::iterator EntryListIterator;
    std::list<Entry> m_entries;
    //pointers into the entry list
    typedef __gnu_cxx::hash_map<vector<const Word*>, EntryListIterator*> ListPointerMap;
    ListPointerMap m_listPointers;
};
#endif

/**
  * This class hierarchy represents the possible changes in the translation effected
  * by the gibbs operators.
  **/
class TranslationDelta {
  public:
    static long lmcalls;
    TranslationDelta(GibbsOperator* g_operator, Sample& sample, const GainFunction* gf): m_score(-1e6), m_gain(-1.0), m_operator(g_operator), m_sample(sample), m_gf(gf) {}
    /**
      Get the absolute score of this delta
    **/
    double getScore() const { return m_score;}
    /**
    Get the absolute score of this delta
    **/
    double getGain() const { return m_gain;}
    /** 
    * Apply to the sample
    **/
    virtual void apply(const TranslationDelta& noChangeDelta) = 0;
   
    Sample& getSample() const {return m_sample;}
    virtual ~TranslationDelta() {}
    void updateWeightedScore();
    const ScoreComponentCollection& getScores() const { return m_scores;}
    const BleuSufficientStats & getGainSufficientStats() {return m_sufficientStats;}
    GibbsOperator* getOperator()  const {return m_operator;} 
    const GainFunction* getGainFunction() const {return m_gf; }
    virtual TranslationDelta* Create() const = 0;
    void setScores(const ScoreComponentCollection& scores)  { m_scores = scores;}
  protected:
    
  Moses::ScoreComponentCollection m_scores;
    double m_score;
    double m_gain;
    GibbsOperator* m_operator;
    Sample& m_sample;
    const GainFunction* m_gf;
  
    //FIXME: The four LM scoring methods should be merged with the three scoring methods for other features.
    /**
      Compute the change in language model score by adding this target phrase
      into the hypothesis at the given target position.
     **/
    
    void  addSingleOptionLanguageModelScore(const TranslationOption* option, const WordsRange& targetSegment);
  
    void  addPairedOptionLanguageModelScore(const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment);
  
    void  addContiguousPairedOptionLMScore(const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange* leftSegment, const WordsRange* rightTargetSegment);
  
    void  addDiscontiguousPairedOptionLMScore(const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange* leftSegment, const WordsRange* rightTargetSegment);
    /**
      * Initialise the scores for the case where only one source-target pair needs to be considered.
     **/
    void initScoresSingleUpdate(const Sample&, const TranslationOption* option, const TargetGap& gap);
    /**
     * Initialise the scores for the case where two (target contiguous) source-target pairs need to be considered.
     * Note that left and right refers to the target order.
     **/
    void initScoresContiguousPairedUpdate(const Sample&, const TranslationOption* leftOption,
                              const TranslationOption* rightOption, const TargetGap& gap);
    
    /** Discontiguous version of above. */
    void initScoresDiscontiguousPairedUpdate(const Sample&, const TranslationOption* leftOption,
                              const TranslationOption* rightOption, const TargetGap& leftGap, 
                              const TargetGap& rightGap);
                              
  
    
    
  private:
#ifdef LM_CACHE
    static std::map<LanguageModel*,LanguageModelCache> m_cache;
#endif
    void calcSufficientStatsAndGain(const std::vector<const Factor*> & sentence);
    BleuSufficientStats m_sufficientStats;
};
 
/**
  * An update that only changes a single source/target phrase pair. May change length of target
  **/
class TranslationUpdateDelta : public virtual TranslationDelta {
  public:
     TranslationUpdateDelta(GibbsOperator* g_operator, Sample& sample, const TranslationOption* option , const TargetGap& gap, const GainFunction* gf);
     virtual void apply(const TranslationDelta& noChangeDelta);
     TranslationUpdateDelta* Create() const;
     const TranslationOption* getOption() const {return m_option;} 
     const TargetGap& getGap() const { return m_gap;} 
  private:
    const TranslationOption* m_option;
    const TargetGap& m_gap;
};

/**
  * An update that merges two source phrases and their corresponding target phrases.
 **/
class MergeDelta : public virtual TranslationDelta {
  public: 
    /**
     * targetWords - the words in the current target sentence
     * option - the source/target phrase to go into the merged segment
     * targetSegment - the location of the target segment
     **/
    MergeDelta(GibbsOperator* g_operator, Sample& sample, const TranslationOption* option, const TargetGap& gap, const GainFunction* gf);
    virtual void apply(const TranslationDelta& noChangeDelta);
    MergeDelta* Create() const;
  const TranslationOption* getOption() const  {return m_option;} 
  const TargetGap& getGap()  const { return m_gap;} 
  private:
    const TranslationOption* m_option;
    const TargetGap& m_gap;
  
};

/**
 * Like TranslationUpdateDelta, except that it updates a pair of source/target phrase pairs.
**/
class PairedTranslationUpdateDelta : public virtual TranslationDelta {
  public: 
    /** Options and gaps in target order */
    PairedTranslationUpdateDelta(GibbsOperator* g_operator, Sample& sample,
        const TranslationOption* leftOption, const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, const GainFunction* gf);
    
    virtual void apply(const TranslationDelta& noChangeDelta);
    PairedTranslationUpdateDelta* Create() const;
    const TranslationOption* getLeftOption()  const {return m_leftOption;} 
    const TranslationOption* getRightOption()  const {return m_rightOption;}  
    const TargetGap& getLeftGap()  const { return m_leftGap;} 
    const TargetGap& getRightGap()  const { return m_rightGap;} 
  
  private:
    const TranslationOption* m_leftOption;
    const TranslationOption* m_rightOption;
    const TargetGap& m_leftGap;
    const TargetGap& m_rightGap;
};

/**
 * Updates the sample by splitting a source phrase and its corresponding target phrase, choosing new options.
 **/
class SplitDelta : public virtual TranslationDelta {
  public:
    /** Options and gaps in target order */
    SplitDelta(GibbsOperator* g_operator, Sample& sample, const TranslationOption* leftOption, const TranslationOption* rightOption, 
    const TargetGap& gap, const GainFunction* gf);
    virtual void apply(const TranslationDelta& noChangeDelta);
    SplitDelta* Create() const;
  const TranslationOption* getLeftOption()  const {return m_leftOption;} 
  const TranslationOption* getRightOption() const  {return m_rightOption;}  
  const TargetGap& getGap()  const { return m_gap;} 
  
  private:
    const TranslationOption* m_leftOption;
    const TranslationOption* m_rightOption;
    const TargetGap& m_gap;
};

/**
  * Switch the translations on the target side.
 **/
class FlipDelta : public virtual TranslationDelta {
  public: 
    /**  Options and gaps in target order */
    FlipDelta(GibbsOperator* g_operator, Sample& sample, const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
              const TargetGap& leftGap, const TargetGap& rightGap, float distortion, const GainFunction* gf);
    
    virtual void apply(const TranslationDelta& noChangeDelta);
    FlipDelta* Create() const;
  const TranslationOption* getLeftOption()  const {return m_leftTgtOption;} 
  const TranslationOption* getRightOption()  const {return m_rightTgtOption;}  
  const TargetGap& getLeftGap() const  { return m_leftGap;} 
  const TargetGap& getRightGap()  const { return m_rightGap;}
  float getTotalDistortion()  const { return m_totalDistortion;}
  private:
    const TranslationOption* m_leftTgtOption;
    const TranslationOption* m_rightTgtOption;
    const TargetGap& m_leftGap;
    const TargetGap& m_rightGap;
    float m_totalDistortion;
    Hypothesis* m_prevTgtHypo;
    Hypothesis* m_nextTgtHypo;
};


} //namespace

