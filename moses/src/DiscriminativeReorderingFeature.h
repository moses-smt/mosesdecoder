// $Id:  $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006-2011 University of Edinburgh

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

#ifndef moses_DiscriminativeReorderingFeature_h
#define moses_DiscriminativeReorderingFeature_h

#include <stdexcept>
#include <string>
#include <vector>

#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>

#include "Factor.h"
#include "FeatureFunction.h"
#include "FFState.h"
#include "WordsRange.h"


namespace Moses {

class TranslationOption;

typedef boost::unordered_set<std::string> vocab_t;

/*
 * The reordering feature is made up of several subfeatures (Templates)
 * each of which has its own state. The state of the DiscriminativeReordering
 * feature is a list of the template states.
 **/

class ReorderingFeatureTemplateState {
  public:
    virtual int Compare(const ReorderingFeatureTemplateState& other) const = 0;
};

typedef boost::shared_ptr<ReorderingFeatureTemplateState> TemplateStateHandle;

class ReorderingFeatureTemplate {
  public:
    ReorderingFeatureTemplate(): m_vocab(NULL) {}
    virtual TemplateStateHandle Evaluate(
            const Hypothesis& cur_hypo,
            const TemplateStateHandle  state,
            const std::string& stem,
            FVector& accumulator) const = 0;
    virtual TemplateStateHandle EmptyState(const InputType &input) const = 0;

    void SetVocab(vocab_t* vocab) {m_vocab = vocab;}
  protected:
    bool CheckVocab(const std::string& token) const;
    virtual ~ReorderingFeatureTemplate() {}

  private:
    vocab_t* m_vocab;
};

class EdgeReorderingFeatureTemplateState: public ReorderingFeatureTemplateState {

  public:
    EdgeReorderingFeatureTemplateState(
          const TranslationOption* toption, bool source, FactorType factor);
    /** empty state */
    EdgeReorderingFeatureTemplateState():
      m_lastFactor(NULL) {}
      
    virtual int Compare(const ReorderingFeatureTemplateState& other) const;
    const Factor* GetLastFactor() const {return m_lastFactor;}

  private:
    const Factor* m_lastFactor;
};


class EdgeReorderingFeatureTemplate : public ReorderingFeatureTemplate {
  public:
    EdgeReorderingFeatureTemplate(size_t factor, bool source, bool curr)
     : m_factor(factor), m_source(source), m_curr(curr) {}

    virtual TemplateStateHandle Evaluate(
            const Hypothesis& cur_hypo,
            const TemplateStateHandle state,
            const std::string& stem,
            FVector& accumulator) const;
    virtual TemplateStateHandle EmptyState(const InputType &input) const;

  private:
    size_t m_factor;
    bool m_source; //source or target?
    bool m_curr; //curr of prev?
};


class DiscriminativeReorderingState: public FFState {
  public:
    DiscriminativeReorderingState();
    DiscriminativeReorderingState(const WordsRange* prevWordsRange);
    virtual int Compare(const FFState& other) const;
    //Implemented to here.
    void AddTemplateState(TemplateStateHandle templateState);
    TemplateStateHandle GetTemplateState(size_t index) const;
    const std::string& GetMsd(const WordsRange& currWordsRange) const;

  private:
    std::vector<TemplateStateHandle> m_templateStates;
    const WordsRange* m_prevWordsRange;
};

class DiscriminativeReorderingFeature : public StatefulFeatureFunction {
  public:
    DiscriminativeReorderingFeature(const std::vector<std::string>& featureConfig,
                                    const std::vector<std::string>& vocabConfig);

    size_t GetNumScoreComponents() const;
    std::string GetScoreProducerWeightShortName() const;
    size_t GetNumInputScores() const;

  	virtual const FFState* EmptyHypothesisState(const InputType &input) const;

	  virtual FFState* Evaluate(const Hypothesis& cur_hypo,
                              const FFState* prev_state,
	                            ScoreComponentCollection* accumulator) const;

  private:
   std::vector<ReorderingFeatureTemplate*> m_templates;
   std::map<FactorType,vocab_t> m_sourceVocabs;
   std::map<FactorType,vocab_t> m_targetVocabs;

   void LoadVocab(std::string filename, vocab_t* vocab);


};

}
#endif

