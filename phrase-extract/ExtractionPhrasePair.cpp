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
#include "SafeGetline.h"
#include "tables-core.h"
#include "score.h"
#include "moses/Util.h"

#include <cstdlib>

using namespace std;


namespace MosesTraining {


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
  assert(phraseSource.empty());
  assert(phraseTarget.empty());

  m_count = count;
  m_pcfgSum = pcfgSum;
  
  std::pair< std::map<ALIGNMENT*,float>::iterator, bool > insertedAlignment =
      m_targetToSourceAlignments.insert( std::pair<ALIGNMENT*,float>(targetToSourceAlignment,count) );

  m_lastTargetToSourceAlignment = insertedAlignment.first;
  m_lastCount = m_count;
  m_lastPcfgSum = m_pcfgSum;

  m_isValid = true;
}


ExtractionPhrasePair::~ExtractionPhrasePair( ) {
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
    assert(keyValue.size() == 2);
    AddProperty(keyValue[0], keyValue[1], count);
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
    if (iter!=allPropertyValues->begin()) {
      oss << " ";
    }
    oss << iter->first;
    oss << " ";
    oss << iter->second;
  }

  std::string allPropertyValuesString(oss.str());
  return allPropertyValuesString;
}


}

