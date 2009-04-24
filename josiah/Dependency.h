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

#include <set>
#include <vector>

#include "Factor.h"

#include "Gibbler.h"
#include "FeatureFunction.h"

namespace Josiah {

class DependencyTree {
  public:
    DependencyTree(const std::vector<Word>& words, Moses::FactorType parentFactor);
    /** Parent of this index, -1 if root*/
    int getParent(size_t index) const;
    /** Does the parent word cover the child word? */
    bool covers(size_t parent, size_t child) const;
    /** length of sentence */
    size_t getLength() const {return m_parents.size();}
    const set<size_t> & getChildren(size_t parent) const { return m_spans[parent];} 
    
  private:
    std::vector<int> m_parents;
    std::vector<std::set<size_t> > m_spans;
};
ostream& operator<<(ostream& out, const DependencyTree& t);

class CherrySyntacticCohesionFeature : public FeatureFunction {
  public:
    CherrySyntacticCohesionFeature(Moses::FactorType parentFactor): FeatureFunction("Cherry"), m_parentFactor(parentFactor) {}
    /** Initialise with new sample */
    virtual void init(const Sample& sample) {
      m_sourceTree.reset(new DependencyTree(sample.GetSourceWords(), m_parentFactor));
      m_sample = &sample;
      //cerr << "New Tree: " << *(m_sourceTree.get()) << endl;
      for (size_t parent = 0; parent < m_sourceTree->getLength(); ++parent) {
        for (size_t child = 0; child < m_sourceTree->getLength(); ++child) {
          //cerr << "parent " << parent << " child " << child << " covers " << m_sourceTree->covers(parent,child) << endl;
        }
      }
    }
    /** Compute full score of a sample from scratch **/
    virtual float computeScore();
    /** Score due to  one segment */
    virtual float getSingleUpdateScore(const TranslationOption* option, const WordsRange& targetSegment);
    /** Score due to two segments **/
    virtual float getPairedUpdateScore(const TranslationOption* leftOption,
                                       const TranslationOption* rightOption, const WordsRange& leftTargetSegment, 
                                       const WordsRange& rightTargetSegment,  const Phrase& targetPhrase);
    
    /** Score due to flip */
    virtual float getFlipUpdateScore(const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
                                     const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
                                     const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment);
    
    virtual ~CherrySyntacticCohesionFeature() {}
  
  private:
    auto_ptr<DependencyTree> m_sourceTree;
    Moses::FactorType m_parentFactor; //which factor is the parent index?
    const Sample* m_sample;
    float getInterruptionCount(const TranslationOption* option, const WordsRange& targetSegment, size_t f);
    bool  notAllWordsCoveredByTree(const TranslationOption* option, size_t parent);
    bool  isInterrupting(Hypothesis* hyp, const WordsRange& targetSegment);
};

}
