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
#include <deque>

#ifdef LM_CACHE
#include <boost/unordered_map.hpp>
#endif

#include "DummyScoreProducers.h"
#include "Gibbler.h"
#include "Hypothesis.h"
#include "TranslationOptionCollection.h"
#include "FeatureFunction.h"

namespace Moses {

class Sample;

#ifdef LM_CACHE
/* 
 * An MRU cache
**/
class LanguageModelCache {
  public:
    LanguageModelCache(LanguageModel* languageModel, int maxSize=10000) :
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
    typedef boost::unordered_map<vector<const Word*>, EntryListIterator*> ListPointerMap;
    ListPointerMap m_listPointers;
};
#endif

/**
  * This class hierarchy represents the possible changes in the translation effected
  * by the gibbs operators.
  **/
class TranslationDelta {
  public:
    TranslationDelta(Sample& sample): m_score(-1e6), m_sample(sample) {
      
    }
  
  /**
    Get the absolute score of this delta
    **/
  double getScore() { return m_score;}
  /** 
    * Apply to the sample
    **/
  virtual void apply(const TranslationDelta& noChangeDelta) = 0;
   
    const ScoreComponentCollection& getScores() const {return m_scores;}
    const std::vector<float>& extra_feature_values() const { return _extra_feature_values; }
    Sample& getSample() const {return m_sample;}
    virtual ~TranslationDelta() {}
    
  protected:
    /**
      Compute the change in language model score by adding this target phrase
      into the hypothesis at the given target position.
     **/
    void  addLanguageModelScore(const Phrase& targetPhrase,const WordsRange& targetSegment);
    /**
      * Initialise the scores for the case where only one source-target pair needs to be considered.
     **/
    void initScoresSingleUpdate(const Sample&, const TranslationOption* option, const WordsRange& targetSegment);
    /**
     * Initialise the scores for the case where two source-target pairs need to be considered.
     **/
    void initScoresPairedUpdate(const Sample&, const TranslationOption* leftOption,
                                const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase);
    ScoreComponentCollection m_scores;
    std::vector<float> _extra_feature_values;
    double m_score;
    
  private:
#ifdef LM_CACHE
    static std::map<LanguageModel*,LanguageModelCache> m_cache;
#endif
    Sample& m_sample;
};
 
/**
  * An update that only changes a single source/target phrase pair. May change length of target
  **/
class TranslationUpdateDelta : public virtual TranslationDelta {
  public:
     TranslationUpdateDelta(Sample& sample, const TranslationOption* option , const WordsRange& targetSegment);
     virtual void apply(const TranslationDelta& noChangeDelta);
     
  private:
    const TranslationOption* m_option;
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
    MergeDelta(Sample& sample, const TranslationOption* option, const WordsRange& targetSegment);
    virtual void apply(const TranslationDelta& noChangeDelta);
  
  private:
    const TranslationOption* m_option;
  
};

/**
 * Like TranslationUpdateDelta, except that it updates a pair of source/target phrase pairs.
**/
class PairedTranslationUpdateDelta : public virtual TranslationDelta {
  public: 
    PairedTranslationUpdateDelta(Sample& sample,
        const TranslationOption* leftOption, const TranslationOption* rightOption, 
        const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment);
    
    virtual void apply(const TranslationDelta& noChangeDelta);
    
  private:
    const TranslationOption* m_leftOption;
    const TranslationOption* m_rightOption;
};

/**
 * Updates the sample by splitting a source phrase and its corresponding target phrase, choosing new options.
 **/
class SplitDelta : public virtual TranslationDelta {
  public:
    SplitDelta(Sample& sample, const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange& targetSegment);
    virtual void apply(const TranslationDelta& noChangeDelta);
    
  private:
    const TranslationOption* m_leftOption;
    const TranslationOption* m_rightOption;
    
};

/**
  * Switch the translations on the target side.
 **/
class FlipDelta : public virtual TranslationDelta {
  public: 
    FlipDelta(Sample& sample, const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
              const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
                                 const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment, float distortion);
    
    virtual void apply(const TranslationDelta& noChangeDelta);
    
  private:
    const TranslationOption* m_leftTgtOption;
    const TranslationOption* m_rightTgtOption;
    Hypothesis* m_prevTgtHypo;
    Hypothesis* m_nextTgtHypo;
};


} //namespace

