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

#include <fstream>

#include "DiscriminativeLMFeature.h"


using namespace Moses;
using namespace std;

namespace Josiah {
  
const string DiscriminativeLMBigramFeature::ROOTNAME = "dlmb";

DiscriminativeLMBigramFeature::DiscriminativeLMBigramFeature
  (FactorType factorId,const std::string& vocabFile) : m_factorId(factorId) {
  if (!vocabFile.empty()) {
    ifstream in(vocabFile.c_str());
    assert(in);
    string line;
    while (getline(in,line)) {
      m_vocab.insert(line);
    }
    m_vocab.insert(EOS_);
    m_vocab.insert(BOS_);
  }

  //create BOS and EOS
  FactorCollection& factorCollection = FactorCollection::Instance();
  const Factor* bosFactor = factorCollection.AddFactor(Input,m_factorId,BOS_);
  BOS.SetFactor(m_factorId,bosFactor);
  const Factor* eosFactor = factorCollection.AddFactor(Input,m_factorId,EOS_);
  EOS.SetFactor(m_factorId,eosFactor);
}


FeatureFunctionHandle DiscriminativeLMBigramFeature::getFunction(const Sample& sample) const {
  return FeatureFunctionHandle(new DiscriminativeLMBigramFeatureFunction(sample,*this));
}

const Word& DiscriminativeLMBigramFeature::bos() const {
  return BOS;
}

const Word& DiscriminativeLMBigramFeature::eos() const {
  return EOS;
}

const std::set<std::string>& DiscriminativeLMBigramFeature::vocab() const {
  return m_vocab;
}

Moses::FactorType DiscriminativeLMBigramFeature::factorId() const {
  return m_factorId;
}

DiscriminativeLMBigramFeatureFunction::DiscriminativeLMBigramFeatureFunction
  (const Sample& sample, const DiscriminativeLMBigramFeature& parent):
    FeatureFunction(sample), m_parent(parent)
{}

void DiscriminativeLMBigramFeatureFunction::updateTarget() {
  m_targetWords = getSample().GetTargetWords();
}

void DiscriminativeLMBigramFeatureFunction::scoreBigram(const Word& word1, const Word& word2, FVector& scores) {
    const string& text1 = word1[m_parent.factorId()]->GetString();
    if (!m_parent.vocab().empty() && m_parent.vocab().find(text1) == m_parent.vocab().end()) {
      return;
    }
    const string& text2 = word2[m_parent.factorId()]->GetString();
    if (!m_parent.vocab().empty() && m_parent.vocab().find(text2) == m_parent.vocab().end()) {
      return;
    }
    FName name(m_parent.ROOTNAME, text1 + ":" + text2);
    ++scores[name];
}



/** Assign the total score of this feature on the current hypo */
void DiscriminativeLMBigramFeatureFunction::assignScore(FVector& scores)
{
    for (size_t i = 0; i < m_targetWords.size()-1; ++i) {
        scoreBigram(m_targetWords[i],m_targetWords[i+1],scores);
    }
}

void DiscriminativeLMBigramFeatureFunction::doUpdate(const Phrase& gapPhrase, const TargetGap& gap, FVector& scores)
{
    if (gap.leftHypo->GetPrevHypo()) {
        //left edge
        const TargetPhrase& leftPhrase = gap.leftHypo->GetTargetPhrase();
        scoreBigram(leftPhrase.GetWord(leftPhrase.GetSize()-1), gapPhrase.GetWord(0),scores);
    } else {
      scoreBigram(m_parent.bos(), gapPhrase.GetWord(0),scores);
    }
    //gap phrase
    size_t i = 0;
    for (; i < gapPhrase.GetSize()-1; ++i) {
        scoreBigram(gapPhrase.GetWord(i), gapPhrase.GetWord(i+1),scores);
    }
    
    //right edge
    if (gap.rightHypo) {
        scoreBigram(gapPhrase.GetWord(i),gap.rightHypo->GetTargetPhrase().GetWord(0), scores);
    } else {
      scoreBigram(gapPhrase.GetWord(i),m_parent.eos(),scores);
    }

    
}

/** Score due to one segment */
void DiscriminativeLMBigramFeatureFunction::doSingleUpdate
        (const TranslationOption* option, const TargetGap& gap, FVector& scores) 
{
    doUpdate(option->GetTargetPhrase(),gap, scores);
}

/** Score due to two segments. The left and right refer to the target positions.**/
void DiscriminativeLMBigramFeatureFunction::doContiguousPairedUpdate
        (const TranslationOption* leftOption,const TranslationOption* rightOption,
            const TargetGap& gap, FVector& scores)
{
    Phrase gapPhrase(leftOption->GetTargetPhrase());
    gapPhrase.Append(rightOption->GetTargetPhrase());
    doUpdate(gapPhrase,gap,scores);
}

void DiscriminativeLMBigramFeatureFunction::doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores)
{
    doUpdate(leftOption->GetTargetPhrase(), leftGap, scores);
    doUpdate(rightOption->GetTargetPhrase(), rightGap, scores);
}

/** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
void DiscriminativeLMBigramFeatureFunction::doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) 
{
    if (leftGap.segment.GetEndPos()+1 == rightGap.segment.GetStartPos()) {
        //contiguous
        Phrase gapPhrase(leftOption->GetTargetPhrase());
        gapPhrase.Append(rightOption->GetTargetPhrase());
        TargetGap gap(leftGap.leftHypo, rightGap.rightHypo, 
                      WordsRange(leftGap.segment.GetStartPos(), rightGap.segment.GetEndPos()));
        doUpdate(gapPhrase,gap,scores);
    } else {
        //discontiguous
        doUpdate(leftOption->GetTargetPhrase(), leftGap,scores);
        doUpdate(rightOption->GetTargetPhrase(), rightGap,scores);
    }
}

}
