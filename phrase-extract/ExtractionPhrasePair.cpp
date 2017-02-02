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

#include <sstream>
#include "ExtractionPhrasePair.h"
#include "tables-core.h"
#include "score.h"
#include "moses/Util.h"

#include <cstdlib>

using namespace std;


namespace MosesTraining
{


extern Vocabulary vcbT;
extern Vocabulary vcbS;

extern bool hierarchicalFlag;


ExtractionPhrasePair::ExtractionPhrasePair( const PHRASE *phraseSource,
    const PHRASE *phraseTarget,
    ALIGNMENT *targetToSourceAlignment,
    float count, float pcfgSum ) :
  m_phraseSource(phraseSource),
  m_phraseTarget(phraseTarget),
  m_count(count),
  m_pcfgSum(pcfgSum)
{
  assert(!phraseSource->empty());

  m_count = count;
  m_pcfgSum = pcfgSum;

  std::pair< std::map<ALIGNMENT*,float>::iterator, bool > insertedAlignment =
    m_targetToSourceAlignments.insert( std::pair<ALIGNMENT*,float>(targetToSourceAlignment,count) );

  m_lastTargetToSourceAlignment = insertedAlignment.first;
  m_lastCount = m_count;
  m_lastPcfgSum = m_pcfgSum;

  m_isValid = true;
}


ExtractionPhrasePair::~ExtractionPhrasePair( )
{
  Clear();
}


// return value: true if the given alignment was seen for the first time and thus will be stored,
//               false if it was present already (the pointer may thus be deleted(
bool ExtractionPhrasePair::Add( ALIGNMENT *targetToSourceAlignment,
                                float count, float pcfgSum )
{
  m_count += count;
  m_pcfgSum += pcfgSum;

  m_lastCount = count;
  m_lastPcfgSum = pcfgSum;

  std::map<ALIGNMENT*,float>::iterator iter = m_lastTargetToSourceAlignment;
  if ( *(iter->first) == *targetToSourceAlignment ) {
    iter->second += count;
    return false;
  } else {
    std::pair< std::map<ALIGNMENT*,float>::iterator, bool > insertedAlignment =
      m_targetToSourceAlignments.insert( std::pair<ALIGNMENT*,float>(targetToSourceAlignment,count) );
    if ( !insertedAlignment.second ) {
      // the alignment already exists: increment count
      insertedAlignment.first->second += count;
      return false;
    }
    m_lastTargetToSourceAlignment = insertedAlignment.first;
  }

  return true;
}


void ExtractionPhrasePair::IncrementPrevious( float count, float pcfgSum )
{
  m_count += count;
  m_pcfgSum += pcfgSum;
  m_lastTargetToSourceAlignment->second += count;
  // properties
  for ( std::map<std::string, std::pair< PROPERTY_VALUES*, LAST_PROPERTY_VALUE* > >::iterator iter=m_properties.begin();
        iter !=m_properties.end(); ++iter ) {
    LAST_PROPERTY_VALUE *lastPropertyValue = (iter->second).second;
    (*lastPropertyValue)->second += count;
  }

  m_lastCount = count;
  m_lastPcfgSum = pcfgSum;
}


// Check for lexical match
// and in case of SCFG rules for equal non-terminal alignment.
bool ExtractionPhrasePair::Matches( const PHRASE *otherPhraseSource,
                                    const PHRASE *otherPhraseTarget,
                                    ALIGNMENT *otherTargetToSourceAlignment ) const
{
  if (*otherPhraseTarget != *m_phraseTarget) {
    return false;
  }
  if (*otherPhraseSource != *m_phraseSource) {
    return false;
  }

  return MatchesAlignment( otherTargetToSourceAlignment );
}

// Check for lexical match
// and in case of SCFG rules for equal non-terminal alignment.
// Set boolean indicators.
// (Note that we check in the order: target - source - alignment
//  and do not touch the subsequent boolean indicators once a previous one has been set to false.)
bool ExtractionPhrasePair::Matches( const PHRASE *otherPhraseSource,
                                    const PHRASE *otherPhraseTarget,
                                    ALIGNMENT *otherTargetToSourceAlignment,
                                    bool &sourceMatch,
                                    bool &targetMatch,
                                    bool &alignmentMatch ) const
{
  if (*otherPhraseSource != *m_phraseSource) {
    sourceMatch = false;
    return false;
  } else {
    sourceMatch = true;
  }
  if (*otherPhraseTarget != *m_phraseTarget) {
    targetMatch = false;
    return false;
  } else {
    targetMatch = true;
  }
  if ( !MatchesAlignment(otherTargetToSourceAlignment) ) {
    alignmentMatch = false;
    return false;
  } else {
    alignmentMatch = true;
  }
  return true;
}

// Check for equal non-terminal alignment in case of SCFG rules.
// Precondition: otherTargetToSourceAlignment has the same size as m_targetToSourceAlignments.begin()->first
bool ExtractionPhrasePair::MatchesAlignment( ALIGNMENT *otherTargetToSourceAlignment ) const
{
  if (!hierarchicalFlag) return true;

  // all or none of the phrasePair's word alignment matrices match, so just pick one
  const ALIGNMENT *thisTargetToSourceAlignment = m_targetToSourceAlignments.begin()->first;

  assert(m_phraseTarget->size() == thisTargetToSourceAlignment->size() + 1);
  assert(thisTargetToSourceAlignment->size() == otherTargetToSourceAlignment->size());

  // loop over all symbols but the left hand side of the rule
  for (size_t i=0; i<thisTargetToSourceAlignment->size()-1; ++i) {
    if (isNonTerminal( vcbT.getWord( m_phraseTarget->at(i) ) )) {
      size_t thisAlign  = *(thisTargetToSourceAlignment->at(i).begin());
      size_t otherAlign = *(otherTargetToSourceAlignment->at(i).begin());

      if (thisTargetToSourceAlignment->at(i).size() != 1 ||
          otherTargetToSourceAlignment->at(i).size() != 1 ||
          thisAlign != otherAlign) {
        return false;
      }
    }
  }

  return true;
}

void ExtractionPhrasePair::Clear()
{
  delete m_phraseSource;
  delete m_phraseTarget;

  m_count = 0.0f;
  m_pcfgSum = 0.0f;

  for ( std::map<ALIGNMENT*,float>::iterator iter=m_targetToSourceAlignments.begin();
        iter!=m_targetToSourceAlignments.end(); ++iter) {
    delete iter->first;
  }
  m_targetToSourceAlignments.clear();

  for ( std::map<std::string, std::pair< PROPERTY_VALUES*, LAST_PROPERTY_VALUE* > >::iterator iter=m_properties.begin();
        iter!=m_properties.end(); ++iter) {
    delete (iter->second).second;
    delete (iter->second).first;
  }
  m_properties.clear();

  m_lastCount = 0.0f;
  m_lastPcfgSum = 0.0f;
  m_lastTargetToSourceAlignment = m_targetToSourceAlignments.begin();

  m_isValid = false;
}


void ExtractionPhrasePair::AddProperties( const std::string &propertiesString, float count )
{
  if (propertiesString.empty()) {
    return;
  }

  vector<std::string> toks;
  Moses::TokenizeMultiCharSeparator(toks, propertiesString, "{{");
  for (size_t i = 1; i < toks.size(); ++i) {
    std::string &tok = toks[i];
    if (tok.empty()) {
      continue;
    }
    size_t endPos = tok.rfind("}");
    tok = tok.substr(0, endPos - 1);

    vector<std::string> keyValue = Moses::TokenizeFirstOnly(tok, " ");
    if (keyValue.size() == 2) {
      AddProperty(keyValue[0], keyValue[1], count);
    }
  }
}


const ALIGNMENT *ExtractionPhrasePair::FindBestAlignmentTargetToSource() const
{
  float bestAlignmentCount = -1;

  std::map<ALIGNMENT*,float>::const_iterator bestAlignment = m_targetToSourceAlignments.end();

  for (std::map<ALIGNMENT*,float>::const_iterator iter=m_targetToSourceAlignments.begin();
       iter!=m_targetToSourceAlignments.end(); ++iter) {
    if ( (iter->second > bestAlignmentCount) ||
         ( (iter->second == bestAlignmentCount) &&
           (*(iter->first) > *(bestAlignment->first)) ) ) {
      bestAlignmentCount = iter->second;
      bestAlignment = iter;
    }
  }

  if ( bestAlignment == m_targetToSourceAlignments.end()) {
    return NULL;
  }

  return bestAlignment->first;
}


const std::string *ExtractionPhrasePair::FindBestPropertyValue(const std::string &key) const
{
  float bestPropertyCount = -1;

  const PROPERTY_VALUES *allPropertyValues = GetProperty( key );
  if ( allPropertyValues == NULL ) {
    return NULL;
  }

  PROPERTY_VALUES::const_iterator bestPropertyValue = allPropertyValues->end();

  for (PROPERTY_VALUES::const_iterator iter=allPropertyValues->begin();
       iter!=allPropertyValues->end(); ++iter) {
    if ( (iter->second > bestPropertyCount) ||
         ( (iter->second == bestPropertyCount) &&
           (iter->first > bestPropertyValue->first) ) ) {
      bestPropertyCount = iter->second;
      bestPropertyValue = iter;
    }
  }

  if ( bestPropertyValue == allPropertyValues->end()) {
    return NULL;
  }

  return &(bestPropertyValue->first);
}


std::string ExtractionPhrasePair::CollectAllPropertyValues(const std::string &key) const
{
  const PROPERTY_VALUES *allPropertyValues = GetProperty( key );

  if ( allPropertyValues == NULL ) {
    return "";
  }

  std::ostringstream oss;
  for (PROPERTY_VALUES::const_iterator iter=allPropertyValues->begin();
       iter!=allPropertyValues->end(); ++iter) {
    if (!(iter->first).empty()) {
      if (iter!=allPropertyValues->begin()) {
        oss << " ";
      }
      oss << iter->first;
      oss << " ";
      oss << iter->second;
    }
  }

  std::string allPropertyValuesString(oss.str());
  return allPropertyValuesString;
}


std::string ExtractionPhrasePair::CollectAllLabelsSeparateLHSAndRHS(const std::string& propertyKey,
    std::set<std::string>& labelSet,
    boost::unordered_map<std::string,float>& countsLabelsLHS,
    boost::unordered_map<std::string, boost::unordered_map<std::string,float>* >& jointCountsRulesTargetLHSAndLabelsLHS,
    Vocabulary &vcbT) const
{
  const PROPERTY_VALUES *allPropertyValues = GetProperty( propertyKey );

  if ( allPropertyValues == NULL ) {
    return "";
  }

  std::string lhs="", rhs="", currentRhs="";
  float currentRhsCount = 0.0;
  std::list< std::pair<std::string,float> > lhsGivenCurrentRhsCounts;

  std::ostringstream oss;
  for (PROPERTY_VALUES::const_iterator iter=allPropertyValues->begin();
       iter!=allPropertyValues->end(); ++iter) {

    size_t space = (iter->first).find_last_of(' ');
    if ( space == string::npos ) {
      lhs = iter->first;
      rhs.clear();
    } else {
      lhs = (iter->first).substr(space+1);
      rhs = (iter->first).substr(0,space);
    }

    labelSet.insert(lhs);

    if ( rhs.compare(currentRhs) ) {

      if ( iter!=allPropertyValues->begin() ) {
        if ( !currentRhs.empty() ) {
          istringstream tokenizer(currentRhs);
          std::string rhsLabel;
          while ( tokenizer.peek() != EOF ) {
            tokenizer >> rhsLabel;
            labelSet.insert(rhsLabel);
          }
          oss << " " << currentRhs << " " << currentRhsCount;
        }
        if ( lhsGivenCurrentRhsCounts.size() > 0 ) {
          if ( !currentRhs.empty() ) {
            oss << " " << lhsGivenCurrentRhsCounts.size();
          }
          for ( std::list< std::pair<std::string,float> >::const_iterator iter2=lhsGivenCurrentRhsCounts.begin();
                iter2!=lhsGivenCurrentRhsCounts.end(); ++iter2 ) {
            oss << " " << iter2->first << " " << iter2->second;

            // update countsLabelsLHS and jointCountsRulesTargetLHSAndLabelsLHS
            std::string ruleTargetLhs = vcbT.getWord(m_phraseTarget->back());
            ruleTargetLhs.erase(ruleTargetLhs.begin());  // strip square brackets
            ruleTargetLhs.erase(ruleTargetLhs.size()-1);

            std::pair< boost::unordered_map<std::string,float>::iterator, bool > insertedCountsLabelsLHS =
              countsLabelsLHS.insert(std::pair<std::string,float>(iter2->first,iter2->second));
            if (!insertedCountsLabelsLHS.second) {
              (insertedCountsLabelsLHS.first)->second += iter2->second;
            }

            boost::unordered_map<std::string, boost::unordered_map<std::string,float>* >::iterator jointCountsRulesTargetLHSAndLabelsLHSIter =
              jointCountsRulesTargetLHSAndLabelsLHS.find(ruleTargetLhs);
            if ( jointCountsRulesTargetLHSAndLabelsLHSIter == jointCountsRulesTargetLHSAndLabelsLHS.end() ) {
              boost::unordered_map<std::string,float>* jointCounts = new boost::unordered_map<std::string,float>;
              jointCounts->insert(std::pair<std::string,float>(iter2->first,iter2->second));
              jointCountsRulesTargetLHSAndLabelsLHS.insert(std::pair<std::string,boost::unordered_map<std::string,float>* >(ruleTargetLhs,jointCounts));
            } else {
              boost::unordered_map<std::string,float>* jointCounts = jointCountsRulesTargetLHSAndLabelsLHSIter->second;
              std::pair< boost::unordered_map<std::string,float>::iterator, bool > insertedJointCounts =
                jointCounts->insert(std::pair<std::string,float>(iter2->first,iter2->second));
              if (!insertedJointCounts.second) {
                (insertedJointCounts.first)->second += iter2->second;
              }
            }

          }
        }

        lhsGivenCurrentRhsCounts.clear();
      }

      currentRhsCount = 0.0;
      currentRhs = rhs;
    }

    currentRhsCount += iter->second;
    lhsGivenCurrentRhsCounts.push_back( std::pair<std::string,float>(lhs,iter->second) );
  }

  if ( !currentRhs.empty() ) {
    istringstream tokenizer(currentRhs);
    std::string rhsLabel;
    while ( tokenizer.peek() != EOF ) {
      tokenizer >> rhsLabel;
      labelSet.insert(rhsLabel);
    }
    oss << " " << currentRhs << " " << currentRhsCount;
  }
  if ( lhsGivenCurrentRhsCounts.size() > 0 ) {
    if ( !currentRhs.empty() ) {
      oss << " " << lhsGivenCurrentRhsCounts.size();
    }
    for ( std::list< std::pair<std::string,float> >::const_iterator iter2=lhsGivenCurrentRhsCounts.begin();
          iter2!=lhsGivenCurrentRhsCounts.end(); ++iter2 ) {
      oss << " " << iter2->first << " " << iter2->second;

      // update countsLabelsLHS and jointCountsRulesTargetLHSAndLabelsLHS
      std::string ruleTargetLhs = vcbT.getWord(m_phraseTarget->back());
      ruleTargetLhs.erase(ruleTargetLhs.begin());  // strip square brackets
      ruleTargetLhs.erase(ruleTargetLhs.size()-1);

      std::pair< boost::unordered_map<std::string,float>::iterator, bool > insertedCountsLabelsLHS =
        countsLabelsLHS.insert(std::pair<std::string,float>(iter2->first,iter2->second));
      if (!insertedCountsLabelsLHS.second) {
        (insertedCountsLabelsLHS.first)->second += iter2->second;
      }

      boost::unordered_map<std::string, boost::unordered_map<std::string,float>* >::iterator jointCountsRulesTargetLHSAndLabelsLHSIter =
        jointCountsRulesTargetLHSAndLabelsLHS.find(ruleTargetLhs);
      if ( jointCountsRulesTargetLHSAndLabelsLHSIter == jointCountsRulesTargetLHSAndLabelsLHS.end() ) {
        boost::unordered_map<std::string,float>* jointCounts = new boost::unordered_map<std::string,float>;
        jointCounts->insert(std::pair<std::string,float>(iter2->first,iter2->second));
        jointCountsRulesTargetLHSAndLabelsLHS.insert(std::pair<std::string,boost::unordered_map<std::string,float>* >(ruleTargetLhs,jointCounts));
      } else {
        boost::unordered_map<std::string,float>* jointCounts = jointCountsRulesTargetLHSAndLabelsLHSIter->second;
        std::pair< boost::unordered_map<std::string,float>::iterator, bool > insertedJointCounts =
          jointCounts->insert(std::pair<std::string,float>(iter2->first,iter2->second));
        if (!insertedJointCounts.second) {
          (insertedJointCounts.first)->second += iter2->second;
        }
      }

    }
  }

  std::string allPropertyValuesString(oss.str());
  return allPropertyValuesString;
}


void ExtractionPhrasePair::CollectAllPhraseOrientations(const std::string &key,
    const std::vector<float> &orientationClassPriorsL2R,
    const std::vector<float> &orientationClassPriorsR2L,
    double smoothingFactor,
    std::ostream &out) const
{
  assert(orientationClassPriorsL2R.size()==4 && orientationClassPriorsR2L.size()==4); // mono swap dleft dright

  const PROPERTY_VALUES *allPropertyValues = GetProperty( key );

  if ( allPropertyValues == NULL ) {
    return;
  }

  // bidirectional MSLR phrase orientation with 2x4 orientation classes:
  // mono swap dright dleft
  std::vector<float> orientationClassCountSumL2R(4,0);
  std::vector<float> orientationClassCountSumR2L(4,0);

  for (PROPERTY_VALUES::const_iterator iter=allPropertyValues->begin();
       iter!=allPropertyValues->end(); ++iter) {
    std::string l2rOrientationClass, r2lOrientationClass;
    try {
      istringstream tokenizer(iter->first);
      tokenizer >> l2rOrientationClass;
      tokenizer >> r2lOrientationClass;
      if ( tokenizer.peek() != EOF ) {
        UTIL_THROW(util::Exception, "ExtractionPhrasePair"
                   << ": Collecting phrase orientations failed. "
                   << "Too many tokens?");
      }
    } catch (const std::exception &e) {
      UTIL_THROW(util::Exception, "ExtractionPhrasePair"
                 << ": Collecting phrase orientations failed. "
                 << "Flawed property value in extract file?");
    }

    int l2rOrientationClassId = -1;
    if (!l2rOrientationClass.compare("mono")) {
      l2rOrientationClassId = 0;
    }
    if (!l2rOrientationClass.compare("swap")) {
      l2rOrientationClassId = 1;
    }
    if (!l2rOrientationClass.compare("dleft")) {
      l2rOrientationClassId = 2;
    }
    if (!l2rOrientationClass.compare("dright")) {
      l2rOrientationClassId = 3;
    }
    if (l2rOrientationClassId == -1) {
      UTIL_THROW(util::Exception, "ExtractionPhrasePair"
                 << ": Collecting phrase orientations failed. "
                 << "Unknown orientation class \"" << l2rOrientationClass << "\"." );
    }
    int r2lOrientationClassId = -1;
    if (!r2lOrientationClass.compare("mono")) {
      r2lOrientationClassId = 0;
    }
    if (!r2lOrientationClass.compare("swap")) {
      r2lOrientationClassId = 1;
    }
    if (!r2lOrientationClass.compare("dleft")) {
      r2lOrientationClassId = 2;
    }
    if (!r2lOrientationClass.compare("dright")) {
      r2lOrientationClassId = 3;
    }
    if (r2lOrientationClassId == -1) {
      UTIL_THROW(util::Exception, "ExtractionPhrasePair"
                 << ": Collecting phrase orientations failed. "
                 << "Unknown orientation class \"" << r2lOrientationClass << "\"." );
    }

    orientationClassCountSumL2R[l2rOrientationClassId] += iter->second;
    orientationClassCountSumR2L[r2lOrientationClassId] += iter->second;
  }

  for (size_t i=0; i<4; ++i) {
    if (i>0) {
      out << " ";
    }
    out << (float)( (smoothingFactor*orientationClassPriorsL2R[i] + orientationClassCountSumL2R[i]) / (smoothingFactor + m_count) );
  }
  for (size_t i=0; i<4; ++i) {
    out << " " << (float)( (smoothingFactor*orientationClassPriorsR2L[i] + orientationClassCountSumR2L[i]) / (smoothingFactor + m_count) );
  }
}


void ExtractionPhrasePair::UpdateVocabularyFromValueTokens(const std::string& propertyKey,
    std::set<std::string>& vocabulary) const
{
  const PROPERTY_VALUES *allPropertyValues = GetProperty( propertyKey );

  if ( allPropertyValues == NULL ) {
    return;
  }

  for (PROPERTY_VALUES::const_iterator iter=allPropertyValues->begin();
       iter!=allPropertyValues->end(); ++iter) {

    std::vector<std::string> tokens = Moses::Tokenize(iter->first);
    for (std::vector<std::string>::const_iterator tokenIt=tokens.begin();
         tokenIt!=tokens.end(); ++tokenIt) {
      vocabulary.insert(*tokenIt);
    }
  }
}



}

