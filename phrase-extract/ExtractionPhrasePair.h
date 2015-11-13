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
#include "tables-core.h"

#include <vector>
#include <set>
#include <map>
#include <boost/unordered_map.hpp>

namespace MosesTraining
{


typedef std::vector< std::set<size_t> > ALIGNMENT;


class ExtractionPhrasePair
{

protected:

  typedef std::map<std::string,float> PROPERTY_VALUES;
  typedef std::map<std::string,float>::iterator LAST_PROPERTY_VALUE;


  bool m_isValid;

  const PHRASE *m_phraseSource;
  const PHRASE *m_phraseTarget;

  float m_count;
  float m_pcfgSum;

  std::map<ALIGNMENT*,float> m_targetToSourceAlignments;
  std::map<std::string,
      std::pair< PROPERTY_VALUES*, LAST_PROPERTY_VALUE* > > m_properties;

  float m_lastCount;
  float m_lastPcfgSum;
  std::map<ALIGNMENT*,float>::iterator m_lastTargetToSourceAlignment;

public:

  ExtractionPhrasePair( const PHRASE *phraseSource,
                        const PHRASE *phraseTarget,
                        ALIGNMENT *targetToSourceAlignment,
                        float count, float pcfgSum );

  ~ExtractionPhrasePair();

  bool Add( ALIGNMENT *targetToSourceAlignment,
            float count, float pcfgSum );

  void IncrementPrevious( float count, float pcfgSum );

  bool Matches( const PHRASE *otherPhraseSource,
                const PHRASE *otherPhraseTarget,
                ALIGNMENT *otherTargetToSourceAlignment ) const;

  bool Matches( const PHRASE *otherPhraseSource,
                const PHRASE *otherPhraseTarget,
                ALIGNMENT *otherTargetToSourceAlignment,
                bool &sourceMatch,
                bool &targetMatch,
                bool &alignmentMatch ) const;

  bool MatchesAlignment( ALIGNMENT *otherTargetToSourceAlignment ) const;

  void Clear();

  bool IsValid() const {
    return m_isValid;
  }


  const PHRASE *GetSource() const {
    return m_phraseSource;
  }

  const PHRASE *GetTarget() const {
    return m_phraseTarget;
  }

  float GetCount() const {
    return m_count;
  }

  float GetPcfgScore() const {
    return m_pcfgSum;
  }

  const size_t GetNumberOfProperties() const {
    return m_properties.size();
  }

  const std::map<std::string,float> *GetProperty( const std::string &key ) const {
    std::map<std::string, std::pair< PROPERTY_VALUES*, LAST_PROPERTY_VALUE* > >::const_iterator iter;
    iter = m_properties.find(key);
    if (iter == m_properties.end()) {
      return NULL;
    } else {
      return iter->second.first;
    }
  }

  const ALIGNMENT *FindBestAlignmentTargetToSource() const;

  const std::string *FindBestPropertyValue(const std::string &key) const;

  std::string CollectAllPropertyValues(const std::string &key) const;

  std::string CollectAllLabelsSeparateLHSAndRHS(const std::string& propertyKey,
      std::set<std::string>& sourceLabelSet,
      boost::unordered_map<std::string,float>& sourceLHSCounts,
      boost::unordered_map<std::string, boost::unordered_map<std::string,float>* >& sourceRHSAndLHSJointCounts,
      Vocabulary &vcbT) const;

  void CollectAllPhraseOrientations(const std::string &key,
                                    const std::vector<float> &orientationClassPriorsL2R,
                                    const std::vector<float> &orientationClassPriorsR2L,
                                    double smoothingFactor,
                                    std::ostream &out) const;

  void UpdateVocabularyFromValueTokens(const std::string& propertyKey,
                                       std::set<std::string>& vocabulary) const;

  void AddProperties(const std::string &str, float count);

  void AddProperty(const std::string &key, const std::string &value, float count) {
    std::map<std::string,
        std::pair< PROPERTY_VALUES*, LAST_PROPERTY_VALUE* > >::iterator iter = m_properties.find(key);
    if ( iter == m_properties.end() ) {
      // key not found: insert property key and value
      PROPERTY_VALUES *propertyValues = new PROPERTY_VALUES();
      std::pair<LAST_PROPERTY_VALUE,bool> insertedProperty = propertyValues->insert( std::pair<std::string,float>(value,count) );
      LAST_PROPERTY_VALUE *lastPropertyValue = new LAST_PROPERTY_VALUE(insertedProperty.first);
      m_properties[key] = std::pair< PROPERTY_VALUES*, LAST_PROPERTY_VALUE* >(propertyValues, lastPropertyValue);
    } else {
      LAST_PROPERTY_VALUE *lastPropertyValue = (iter->second).second;
      if ( (*lastPropertyValue)->first == value ) { // same property key-value pair has been seen right before
        // property key-value pair exists already: add count
        (*lastPropertyValue)->second += count;
      } else { // need to check whether the property key-value pair has appeared before (insert if not)
        // property key exists, but not in combination with this value:
        // add new value with count
        PROPERTY_VALUES *propertyValues = (iter->second).first;
        std::pair<LAST_PROPERTY_VALUE,bool> insertedProperty = propertyValues->insert( std::pair<std::string,float>(value,count) );
        if ( !insertedProperty.second ) { // property value for this key appeared before: add count
          insertedProperty.first->second += count;
        }
        LAST_PROPERTY_VALUE *lastPropertyValue = new LAST_PROPERTY_VALUE(insertedProperty.first);
        delete (iter->second).second;
        (iter->second).second = lastPropertyValue;
      }
    }
  }

};

}

