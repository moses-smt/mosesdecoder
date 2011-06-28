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

#include "DiscriminativeReorderingFeature.h"
#include "TargetPhrase.h"
#include "TranslationOption.h"

using namespace std;

namespace Moses {

bool ReorderingFeatureTemplate::CheckVocab(const std::string& token)  const
{
  if (!m_vocab) return true;
  return m_vocab->find(token) != m_vocab->end();
}

EdgeReorderingFeatureTemplateState::EdgeReorderingFeatureTemplateState(
      const TranslationOption* toption, bool source, FactorType factor) 
{
  const Phrase* phrase = NULL;
  if (source) {
    phrase = toption->GetSourcePhrase();
  } else {
    phrase = &(toption->GetTargetPhrase());
  }
  const Word& lastWord = phrase->GetWord(phrase->GetSize()-1);
  m_lastFactor = lastWord.GetFactor(factor);
}

int EdgeReorderingFeatureTemplateState::Compare
    (const ReorderingFeatureTemplateState& other) const 
{
  if (&other == this) return 0;
  const EdgeReorderingFeatureTemplateState* otherState = 
    dynamic_cast<const EdgeReorderingFeatureTemplateState*>(&other);
  assert(otherState);
  if (m_lastFactor == otherState->m_lastFactor) return 0;
  if (!m_lastFactor) return -1;
  if (!otherState->m_lastFactor) return 0;
  return m_lastFactor->Compare(*(otherState->m_lastFactor));
}

TemplateStateHandle EdgeReorderingFeatureTemplate::EmptyState(const InputType&) const 
{
  return TemplateStateHandle(new EdgeReorderingFeatureTemplateState());
}

TemplateStateHandle EdgeReorderingFeatureTemplate::Evaluate(
        const Hypothesis& cur_hypo,
        const TemplateStateHandle state,
        const string& prefix,
        FVector& accumulator) const 
{
  static const string sourcePrev = "s:p:";
  static const string sourceCurr = "s:c:";
  static const string targetPrev = "t:p:";
  static const string targetCurr = "t:c:";

  const EdgeReorderingFeatureTemplateState* edgeState = 
    dynamic_cast<const EdgeReorderingFeatureTemplateState*>(state.get());
  assert(edgeState);

  const TranslationOption& currOption = cur_hypo.GetTranslationOption();

  const Factor* edge = NULL;
  const string* position = NULL;
  if (m_source && m_curr) {
    edge = currOption.GetSourcePhrase()->GetWord(0).GetFactor(m_factor);
    position = &sourceCurr;
  } else if (m_source && !m_curr) {
    edge = edgeState->GetLastFactor(); 
    position = &sourcePrev;
  } else if (!m_source && m_curr) {
    edge = currOption.GetTargetPhrase().GetWord(0).GetFactor(m_factor);
    position = &targetCurr;
  } else {
    edge = edgeState->GetLastFactor(); 
    position = &targetPrev;
  }

  if (!edge || CheckVocab(edge->GetString())) {
    ostringstream namestr;
    namestr << *position;
    namestr << m_factor;
    namestr << ":";
    if (edge) {
      const string& word = edge->GetString();
      namestr << word;
    } else {
      namestr << BOS_;
    }
    FName name(prefix,namestr.str());
    ++accumulator[name];
  }
  return TemplateStateHandle
    (new EdgeReorderingFeatureTemplateState(&currOption,m_source,m_factor));
}

DiscriminativeReorderingState::DiscriminativeReorderingState() :
  m_prevWordsRange(NULL) {}

DiscriminativeReorderingState::DiscriminativeReorderingState(const WordsRange* prevWordsRange) :
  m_prevWordsRange(prevWordsRange) {}


int DiscriminativeReorderingState::Compare(const FFState& other)  const
{
  if (this == &other) return 0;
  const DiscriminativeReorderingState* otherState = 
    dynamic_cast<const DiscriminativeReorderingState*>(&other);
  assert(otherState);
  if (!m_prevWordsRange) {
    if (!otherState->m_prevWordsRange) {
      return 0;
    } else {
      return -1;
    }
  }
  if (!otherState->m_prevWordsRange) {
    return -1;
  }
  if (m_prevWordsRange < otherState->m_prevWordsRange) {
    return -1;
  }
  if (otherState->m_prevWordsRange < m_prevWordsRange) {
    return 1;
  }

  assert(m_templateStates.size() == otherState->m_templateStates.size());
  for (size_t i = 0; i < m_templateStates.size(); ++i) {
    int cmp = m_templateStates[i]->Compare(*(otherState->m_templateStates[i]));
    if (cmp) return cmp;
  }
  return 0;
}

void DiscriminativeReorderingState::AddTemplateState(TemplateStateHandle templateState) 
{
  m_templateStates.push_back(templateState);
}

TemplateStateHandle DiscriminativeReorderingState::GetTemplateState(size_t index) const
{
  return m_templateStates.at(index);
}

const string& DiscriminativeReorderingState::GetMsd(const WordsRange& currWordsRange) const 
{
  int prevStart = -1;
  int prevEnd = -1;
  if (m_prevWordsRange) {
    prevStart = m_prevWordsRange->GetStartPos();
    prevEnd = m_prevWordsRange->GetEndPos();
  }
  int currStart = currWordsRange.GetStartPos();
  int currEnd = currWordsRange.GetEndPos();
  static string monotone = "msd:m";
  static string swap = "msd:s";
  static string discontinuous = "msd:d";
  if (prevEnd + 1 == currStart) {
    return monotone;
  } else if (currEnd + 1 == prevStart) {
    return swap;
  } else {
    return discontinuous;
  }
 
}


DiscriminativeReorderingFeature::DiscriminativeReorderingFeature
  (const vector<string>& featureConfig,
    const vector<string>& vocabConfig) :
    StatefulFeatureFunction("dreo") 
{
  const static string SOURCE = "source";
  const static string TARGET = "target";
  const static string PREV = "prev";
  const static string CURR = "curr";

  //load vocabularies
  for (vector<string>::const_iterator i = vocabConfig.begin(); i != vocabConfig.end();
    ++i) {
    vector<string> vc = Tokenize(*i);
    if (vc.size() != 3) {
      ostringstream errmsg;
      errmsg << "Discrim reordering configuration '" << *i << "' has incorrect format";
      throw runtime_error(errmsg.str());
    }
    FactorType factorId = Scan<FactorType>(vc[0]);
    bool source = true;
    if (vc[1] == TARGET) {
      source = false;
    } else if (vc[1] != SOURCE) {
      throw runtime_error("Discrim reordering vocab config has invalid source/target identifier");
    }
    string filename = vc[2];
    vocab_t* vocab = NULL;
    if (source) {
      vocab = &(m_sourceVocabs[factorId]);
    } else {
      vocab = &(m_targetVocabs[factorId]);
    }

    LoadVocab(filename,vocab);
  }

  for (vector<string>::const_iterator i = featureConfig.begin(); i != featureConfig.end(); ++i)   {
    vector<string> fc = Tokenize(*i);
    if (fc.size() != 4) {
      ostringstream errmsg;
      errmsg << "Discrim reordering feature configuration '" << *i << "' has incorrect format";
      throw runtime_error(errmsg.str());
    }
    size_t factorId = Scan<FactorType>(fc[1]);
    bool source = true;
    if (fc[2] == TARGET) {
      source = false;
    } else if (fc[2] != SOURCE) {
      throw runtime_error("Discrim reordering feature config has invalid source/target identifier");
    }
    bool curr = true;
    if (fc[3] == PREV) {
      curr = false;
    } else if (fc[3] != CURR) {
      throw runtime_error("Discrim reordering config has invalid curr/prev identifier");
    }
    if (fc[0] == "edge") {
      m_templates.push_back(new EdgeReorderingFeatureTemplate(factorId,source,curr));
    } else {
      ostringstream errmsg;
      errmsg << "Unknown msd feature type '" << fc[0] << "'" << endl;
      throw runtime_error(errmsg.str());
    }

    //set vocabulary, if necessary
    vocab_t* vocab = NULL;
    if (source) {
      if (m_sourceVocabs.find(factorId) != m_sourceVocabs.end()) {
        vocab = &(m_sourceVocabs[factorId]);
      }
    } else {
      if (m_targetVocabs.find(factorId) != m_targetVocabs.end()) {
        vocab = &(m_targetVocabs[factorId]);
      }
    }
    m_templates.back()->SetVocab(vocab);
  }


}

size_t DiscriminativeReorderingFeature::GetNumScoreComponents() const 
{
  return unlimited;
}

std::string DiscriminativeReorderingFeature::GetScoreProducerWeightShortName()
   const
{
  return "dreo";
}

size_t DiscriminativeReorderingFeature::GetNumInputScores() const 
{
  return 0;
}

const FFState* DiscriminativeReorderingFeature::EmptyHypothesisState
  (const InputType &input) const
{
  DiscriminativeReorderingState* state = new DiscriminativeReorderingState();
  for (size_t i = 0; i < m_templates.size(); ++i) {
    state->AddTemplateState(m_templates[i]->EmptyState(input));
  }
  return state;
}

FFState* DiscriminativeReorderingFeature::Evaluate(const Hypothesis& cur_hypo,
                          const FFState* prev_state,
                          ScoreComponentCollection* accumulator) const
{
  const DiscriminativeReorderingState* state = 
    dynamic_cast<const DiscriminativeReorderingState*>(prev_state);
  assert(state);
  FVector scores;
  DiscriminativeReorderingState* newState = new DiscriminativeReorderingState
    (&(cur_hypo.GetCurrSourceWordsRange()));
  string stem = state->GetMsd(cur_hypo.GetCurrSourceWordsRange());
  for (size_t i = 0; i < m_templates.size(); ++i) {
    newState->AddTemplateState(m_templates[i]->Evaluate(
        cur_hypo, state->GetTemplateState(i), stem, scores));
  }
  accumulator->PlusEquals(scores);
  return newState;
}

void DiscriminativeReorderingFeature::LoadVocab(string filename, vocab_t* vocab) 
{
  VERBOSE(1, "Loading vocabulary for reordering feature from " << filename << endl);
  vocab->clear();
  ifstream in(filename.c_str());
  if (!in) {
    ostringstream errmsg;
    errmsg << "Unable to load vocabulary from " << filename;
    throw runtime_error(errmsg.str());
  }
  string line;
  while (getline(in,line)) {
    vocab->insert(line);
  }

}

}
