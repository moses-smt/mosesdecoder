/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

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

#include <stdexcept>
#include <fstream>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include "ReorderingFeature.h"

#include "Gibbler.h"
#include "Util.h"

using namespace Moses;
using namespace std;
using boost::lexical_cast;

namespace Josiah {
  
  string ReorderingFeatureTemplate::BOS = "<s>";


ReorderingFeature::ReorderingFeature(const vector<string>& msd,
    const std::vector<std::string>& msdVocab) 
  {

  const static string SOURCE = "source";
  const static string TARGET = "target";
  const static string PREV = "prev";
  const static string CURR = "curr";

  for (vector<string>::const_iterator i = msdVocab.begin(); i != msdVocab.end();
    ++i) {
    vector<string> msdVocabConfig = Tokenize(*i,":");
    if (msdVocabConfig.size() != 3) {
      ostringstream errmsg;
      errmsg << "msdvocab configuration '" << *i << "' has incorrect format";
      throw runtime_error(errmsg.str());
    }
    size_t factorId = lexical_cast<size_t>(msdVocabConfig[0]);
    bool source = true;
    if (msdVocabConfig[1] == TARGET) {
      source = false;
    } else if (msdVocabConfig[1] != SOURCE) {
      throw runtime_error("msd vocab config has invalid source/target identifier");
    }
    string filename = msdVocabConfig[2];
    vocab_t* vocab = NULL;
    if (source) {
      vocab = &(m_sourceVocabs[factorId]);
    } else {
      vocab = &(m_targetVocabs[factorId]);
    }

    loadVocab(filename,vocab);
  }

  for (vector<string>::const_iterator i = msd.begin(); i != msd.end(); ++i) {
    vector<string> msdConfig = Tokenize(*i,":");
    if (msdConfig.size() != 4) {
      ostringstream errmsg;
      errmsg << "msd configuration '" << *i << "' has incorrect format";
      throw runtime_error(errmsg.str());
    }
    size_t factorId = lexical_cast<size_t>(msdConfig[1]);
    bool source = true;
    if (msdConfig[2] == TARGET) {
      source = false;
    } else if (msdConfig[2] != SOURCE) {
      throw runtime_error("msd config has invalid source/target identifier");
    }
    bool curr = true;
    if (msdConfig[3] == PREV) {
      curr = false;
    } else if (msdConfig[3] != CURR) {
      throw runtime_error("msd config has invalid curr/prev identifier");
    }
    if (msdConfig[0] == "edge") {
      m_templates.push_back(new EdgeReorderingFeatureTemplate(factorId,source,curr));
    } else {
      ostringstream errmsg;
      errmsg << "Unknown msd feature type '" << msdConfig[0] << "'" << endl;
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
    m_templates.back()->setVocab(vocab);
  }
}

FeatureFunctionHandle ReorderingFeature::getFunction(const Sample& sample) const {
  return FeatureFunctionHandle(new ReorderingFeatureFunction(sample, *this));
}
  

const std::vector<ReorderingFeatureTemplate*>& ReorderingFeature::getTemplates() const {
  return m_templates;
}

void ReorderingFeature::loadVocab(string filename, vocab_t* vocab) {
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

bool ReorderingFeatureTemplate::checkVocab(const std::string& word) const {
  if (!m_vocab) return true;
  return m_vocab->find(word) != m_vocab->end();
}

 ReorderingFeatureFunction::ReorderingFeatureFunction(const Sample& sample, const ReorderingFeature& parent) 
  : FeatureFunction(sample), m_parent(parent)
{}

/** Assign features for the following options, assuming they are contiguous on the target side */
void  ReorderingFeatureFunction::assign(const TranslationOption* prevOption, const TranslationOption* currOption, FVector& scores) {
  for (vector<ReorderingFeatureTemplate*>::const_iterator i = m_parent.getTemplates().begin(); 
        i != m_parent.getTemplates().end(); ++i) {
    (*i)->assign(prevOption,currOption,getMsd(prevOption, currOption), scores);
  }
}


const string&  ReorderingFeatureFunction::getMsd(const TranslationOption* prevOption, const TranslationOption* currOption) {
  int prevStart = -1;
  int prevEnd = -1;
  if (prevOption) {
    prevStart = prevOption->GetSourceWordsRange().GetStartPos();
    prevEnd = prevOption->GetSourceWordsRange().GetEndPos();
  }
  int currStart = currOption->GetSourceWordsRange().GetStartPos();
  int currEnd = currOption->GetSourceWordsRange().GetEndPos();
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

void EdgeReorderingFeatureTemplate::assign(const Moses::TranslationOption* prevOption, const Moses::TranslationOption* currOption,
                                          const std::string& prefix, FVector& scores) 
{
  static const string sourcePrev = "s:p:";
  static const string sourceCurr = "s:c:";
  static const string targetPrev = "t:p:";
  static const string targetCurr = "t:c:";
  
  const Word* edge = NULL;
  const string* position = NULL;
  if (m_source && m_curr) {
    edge = &(currOption->GetSourcePhrase()->GetWord(0));
    position = &sourceCurr;
  } else if (m_source && !m_curr) {
    if (prevOption) {
      const Phrase* sourcePhrase = prevOption->GetSourcePhrase();
      edge = &(sourcePhrase->GetWord(sourcePhrase->GetSize()-1));
    }
    position = &sourcePrev;
  } else if (!m_source && m_curr) {
    edge = &(currOption->GetTargetPhrase().GetWord(0));
    position = &targetCurr;
  } else {
    if (prevOption) {
      const Phrase& targetPhrase = prevOption->GetTargetPhrase();
      edge = &(targetPhrase.GetWord(targetPhrase.GetSize()-1));
    }
    position = &targetPrev;
  }
  
  ostringstream namestr;
  namestr << *position;
  namestr << m_factor;
  namestr << ":";
  if (edge) {
    const string& word = edge->GetFactor(m_factor)->GetString();
    if (!checkVocab(word)) return;
    namestr << word;
  } else {
    namestr << BOS;
  }
  FName name(prefix,namestr.str());
  ++scores[name];
}


/** Assign the total score of this feature on the current hypo */
void ReorderingFeatureFunction::assignScore(FVector& scores) 
{
  const Hypothesis* currHypo = getSample().GetTargetTail();
  const TranslationOption* prevOption = NULL;
  while ((currHypo = (currHypo->GetNextHypo()))) {
    const TranslationOption* currOption = &(currHypo->GetTranslationOption());
    assign(prevOption,currOption,scores);
    prevOption = currOption;
  }
}
    
/** Score due to one segment */
void ReorderingFeatureFunction::doSingleUpdate(const TranslationOption* option, const TargetGap& gap, FVector& scores)
{
  if (gap.leftHypo) {
    assign(&(gap.leftHypo->GetTranslationOption()), option, scores);
  }
  if (gap.rightHypo) {
    assign(option,&(gap.rightHypo->GetTranslationOption()), scores);
  }
}

/** Score due to two segments. The left and right refer to the target positions.**/
void ReorderingFeatureFunction::doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                      const TargetGap& gap, FVector& scores) 
{
  if (gap.leftHypo) {
    assign(&(gap.leftHypo->GetTranslationOption()), leftOption,scores);
  }
  assign(leftOption,rightOption,scores);
  if (gap.rightHypo) {
    assign(rightOption, &(gap.rightHypo->GetTranslationOption()), scores);
  }
  
}

void ReorderingFeatureFunction::doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                          const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores)
{
  if (leftGap.leftHypo) {
    assign(&(leftGap.leftHypo->GetTranslationOption()),leftOption,scores);
  }
  assign(leftOption, &(leftGap.rightHypo->GetTranslationOption()), scores);
  assign(&(rightGap.leftHypo->GetTranslationOption()),rightOption,scores);
  if (rightGap.rightHypo) {
    assign(rightOption, &(rightGap.rightHypo->GetTranslationOption()),scores);
  }
}

/** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
void ReorderingFeatureFunction::doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                          const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores)
{
  if (leftGap.segment.GetEndPos()+1 == rightGap.segment.GetStartPos()) {
    TargetGap gap(leftGap.leftHypo, rightGap.rightHypo, WordsRange(leftGap.segment.GetStartPos(),rightGap.segment.GetEndPos()));
    doContiguousPairedUpdate(leftOption,rightOption,gap,scores);
  } else {
    doDiscontiguousPairedUpdate(leftOption,rightOption,leftGap,rightGap,scores);
  }
}



}

