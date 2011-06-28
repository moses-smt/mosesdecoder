/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011 University of Edinburgh

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

#include "Gibbler.h"
#include "PhraseBoundaryFeature.h"

using namespace Moses;
using namespace std;

namespace Josiah {
  const string PhraseBoundaryFeature::STEM = "pb";
  const string PhraseBoundaryFeature::SEP = ":";
  const string PhraseBoundaryFeature::SOURCE = "src";
  const string PhraseBoundaryFeature::TARGET = "tgt";
  const string PhraseBoundaryFeature::BOS = "<s>";
  const string PhraseBoundaryFeature::EOS = "</s>";

  PhraseBoundaryFeature::PhraseBoundaryFeature(
    const FactorList& sourceFactors,
    const FactorList& targetFactors,
    const vector<string>& sourceVocabFiles,
    const vector<string>& targetVocabFiles) :
      m_sourceFactors(sourceFactors), m_targetFactors(targetFactors) 
    {
      assert(sourceFactors.size() == sourceVocabFiles.size());  
      assert(targetFactors.size() == targetVocabFiles.size());  
      m_sourceVocabs.resize(sourceVocabFiles.size());
      for (size_t i = 0; i < sourceVocabFiles.size(); ++i) {
        loadVocab(sourceVocabFiles[i],m_sourceVocabs[i]);
      }
      m_targetVocabs.resize(targetVocabFiles.size());
      for (size_t i = 0; i < targetVocabFiles.size(); ++i) {
        loadVocab(targetVocabFiles[i],m_targetVocabs[i]);
      }
    }

  FeatureFunctionHandle PhraseBoundaryFeature::getFunction(const Sample& sample) const {
    return FeatureFunctionHandle(new PhraseBoundaryFeatureFunction(sample,*this));
  }

  void PhraseBoundaryFeature::addSourceFeatures(
    const Word* leftWord, const Word* rightWord, FVector& scores) const {
    addFeatures(leftWord,rightWord,m_sourceFactors,SOURCE, m_sourceVocabs,scores);
  }
  
  void PhraseBoundaryFeature::addTargetFeatures(
    const Word* leftWord, const Word* rightWord, FVector& scores) const {
    addFeatures(leftWord,rightWord,m_targetFactors,TARGET, m_targetVocabs,scores);
  }       

  void PhraseBoundaryFeature::addFeatures(
    const Word* leftWord, const Word* rightWord,
       const FactorList& factors, const string& side,
       const vector<set<string> >& vocabs,  FVector& scores) const 
  {
    for (size_t i = 0; i < factors.size(); ++i) {
      ostringstream name;
      name << side << SEP;
      name << factors[i];
      name << SEP;
      if (leftWord) {
        const string& leftWordText = leftWord->GetFactor(factors[i])->GetString();
        if (vocabs[i].size() != 0 && 
            vocabs[i].find(leftWordText) == vocabs[i].end()) {
          continue;
        }
        name << leftWordText;
      } else {
        name << BOS;
      }
      name << SEP;
      if (rightWord) {
        const string& rightWordText = rightWord->GetFactor(factors[i])->GetString();
        if (vocabs[i].size() != 0 &&
            vocabs[i].find(rightWordText) == vocabs[i].end()) {
          continue;
        }
        name << rightWordText;
      } else {
        name << EOS;
      }
      FName fName(STEM,name.str());
      ++scores[fName];
    }
  }

  PhraseBoundaryFeatureFunction::PhraseBoundaryFeatureFunction
    (const Sample& sample, const PhraseBoundaryFeature& parent) :
    FeatureFunction(sample), m_parent(parent) {}
    
    /** Update the target words.*/
    void PhraseBoundaryFeatureFunction::updateTarget() {}
    
    /** Assign the total score of this feature on the current hypo */
    void PhraseBoundaryFeatureFunction::assignScore(FVector& scores) {
      const Hypothesis* currHypo = getSample().GetTargetTail();
      scoreOptions(NULL,&(currHypo->GetNextHypo()->GetTranslationOption()),scores);
      while ((currHypo = (currHypo->GetNextHypo()))) {
        const TranslationOption* leftOption = 
          &(currHypo->GetTranslationOption());
        const TranslationOption* rightOption = NULL;
        if (currHypo->GetNextHypo()) {
          rightOption = &(currHypo->GetNextHypo()->GetTranslationOption());
        }
        scoreOptions(leftOption,rightOption,scores);          
      }
    }

    /** Score due to one segment */
     void PhraseBoundaryFeatureFunction::doSingleUpdate(
      const TranslationOption* option, const TargetGap& gap, FVector& scores) {
      const TranslationOption* leftOption = NULL;
      if (gap.leftHypo->GetPrevHypo()) {
        leftOption = &(gap.leftHypo->GetTranslationOption());
      }
      const TranslationOption* rightOption = option;
      scoreOptions(leftOption,rightOption,scores);

      leftOption = option;
      if (gap.rightHypo) {
        rightOption = &(gap.rightHypo->GetTranslationOption());
      } else {
        rightOption = NULL;
      }
      scoreOptions(leftOption,rightOption,scores);
    }

    /** Score due to two segments. The left and right refer to the target positions.**/
    void PhraseBoundaryFeatureFunction::doContiguousPairedUpdate(
      const TranslationOption* leftOption,const TranslationOption* rightOption, 
      const TargetGap& gap, FVector& scores) 
    {
      const TranslationOption* prevOption = NULL;
      if (gap.leftHypo->GetPrevHypo()) {
        prevOption = &(gap.leftHypo->GetTranslationOption());
      }
      const TranslationOption* nextOption = NULL;
      if (gap.rightHypo) {
        nextOption = &(gap.rightHypo->GetTranslationOption());
      }
      scoreOptions(prevOption,leftOption,scores);
      scoreOptions(leftOption,rightOption,scores);
      scoreOptions(rightOption,nextOption,scores);
    }

    void PhraseBoundaryFeatureFunction::doDiscontiguousPairedUpdate(
      const TranslationOption* leftOption,const TranslationOption* rightOption, 
      const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) {

      doSingleUpdate(leftOption,leftGap,scores);
      doSingleUpdate(rightOption,rightGap,scores);
    }

    /** Score due to flip. Again, left and right refer to order on the
         <emph>target</emph> side. */
    void PhraseBoundaryFeatureFunction::doFlipUpdate(
      const TranslationOption* leftOption,const TranslationOption* rightOption, 
      const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) {
      if (leftGap.segment.GetEndPos()+1 == rightGap.segment.GetStartPos()) {
        //contiguous
        TargetGap gap(leftGap.leftHypo, rightGap.rightHypo, 
            WordsRange(leftGap.segment.GetStartPos(),
                       rightGap.segment.GetEndPos()));
        doContiguousPairedUpdate(leftOption,rightOption,gap,scores);
      } else {
        //discontiguous
        doDiscontiguousPairedUpdate(leftOption,rightOption,
          leftGap,rightGap,scores);
      }
    }

    static const Word* getBeginWord(const Phrase& phrase) {
      return &(*phrase.begin());
    }

    static const Word* getEndWord(const Phrase& phrase) {
      return &(phrase.GetWord(phrase.GetSize()-1));
    }

    void PhraseBoundaryFeatureFunction::scoreOptions(
      const TranslationOption* leftOption, const TranslationOption* rightOption,
        FVector& scores)
    {
      //source
      const Word* leftSourceWord = NULL;
      const Word* rightSourceWord = NULL;
      if (leftOption) {
        leftSourceWord = getEndWord(*(leftOption->GetSourcePhrase()));
      }
      if (rightOption) {
        rightSourceWord = getBeginWord(*(rightOption->GetSourcePhrase()));
      }
      m_parent.addSourceFeatures(leftSourceWord,rightSourceWord,scores);

      //target
      const Word* leftTargetWord = NULL;
      const Word* rightTargetWord = NULL;
      if (leftOption) {
        leftTargetWord = getEndWord(leftOption->GetTargetPhrase());
      }
      if (rightOption) {
        rightTargetWord = getBeginWord(rightOption->GetTargetPhrase());
      }
      m_parent.addTargetFeatures(leftTargetWord,rightTargetWord,scores);
    }

    void PhraseBoundaryFeature::loadVocab(
     const std::string& filename, std::set<std::string>& vocab) {
      if (filename.empty()) return;
      ifstream in(filename.c_str());
      assert(in);
      string line;
      while(getline(in,line)) {
       vocab.insert(line);
      }
    }

}

