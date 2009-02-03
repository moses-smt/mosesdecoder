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

#include "DummyScoreProducers.h"
#include "Gibbler.h"
#include "Hypothesis.h"
#include "TranslationOptionCollection.h"

namespace Moses {

class Sample;

/**
  * This class hierarchy represents the possible changes in the translation effected
  * by the gibbs operators.
  **/
class TranslationDelta {
  public:
    TranslationDelta(): m_score(-1e6) {
      
    }
  
  /**
    Get the absolute score of this delta
    **/
  double getScore() { return m_score;}
  /** 
    * Apply to the sample
    **/
  virtual void apply(Sample& sample, const TranslationDelta& noChangeDelta) = 0;
   
    const ScoreComponentCollection& getScores() const {return m_scores;}
    virtual ~TranslationDelta() {}
    
  protected:
    /**
      Compute the change in language model score by adding this target phrase
      into the hypothesis at the given target position.
     **/
    void  addLanguageModelScore(const vector<Word>& targetWords, const Phrase& targetPhrase,
                                const WordsRange& targetSegment);
    /**
      * Initialise the scores for the case where only one source-target pair needs to be considered.
     **/
    void initScoresSingleUpdate(const vector<Word>& targetWords, const TranslationOption* option, const WordsRange& targetSegment);
    /**
     * Initialise the scores for the case where two source-target pairs need to be considered.
     **/
    void initScoresPairedUpdate(const vector<Word>& targetWords, const TranslationOption* leftOption,
                                const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase);
    ScoreComponentCollection m_scores;
    double m_score;
    
};
 
/**
  * An update that only changes a single source/target phrase pair. May change length of target
  **/
class TranslationUpdateDelta : public virtual TranslationDelta {
  public:
     TranslationUpdateDelta(const vector<Word>& targetWords,  const TranslationOption* option , const WordsRange& targetSegment);
     virtual void apply(Sample& sample, const TranslationDelta& noChangeDelta);
     
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
    MergeDelta(const vector<Word>& targetWords, const TranslationOption* option, const WordsRange& targetSegment);
    virtual void apply(Sample& sample, const TranslationDelta& noChangeDelta);
  
  private:
    const TranslationOption* m_option;
  
};

/**
 * Like TranslationUpdateDelta, except that it updates a pair of source/target phrase pairs.
**/
class PairedTranslationUpdateDelta : public virtual TranslationDelta {
  public: 
    PairedTranslationUpdateDelta(const vector<Word>& targetWords,
        const TranslationOption* leftOption, const TranslationOption* rightOption, 
        const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment);
    
    virtual void apply(Sample& sample, const TranslationDelta& noChangeDelta);
    
  private:
    const TranslationOption* m_leftOption;
    const TranslationOption* m_rightOption;
};

/**
 * Updates the sample by splitting a source phrase and its corresponding target phrase, choosing new options.
 **/
class SplitDelta : public virtual TranslationDelta {
  public:
    SplitDelta(const vector<Word>& targetWords,
               const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange& targetSegment);
    virtual void apply(Sample& sample, const TranslationDelta& noChangeDelta);
    
  private:
    const TranslationOption* m_leftOption;
    const TranslationOption* m_rightOption;
    
};

/**
  * Switch the translations on the target side.
 **/
class FlipDelta : public virtual TranslationDelta {
  public: 
    FlipDelta(const vector<Word>& targetWords,
                                 const TranslationOption* leftOption, const TranslationOption* rightOption, 
                                 const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment);
    
    virtual void apply(Sample& sample, const TranslationDelta& noChangeDelta);
    
  private:
    const TranslationOption* m_leftOption;
    const TranslationOption* m_rightOption;
};


} //namespace

