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
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "FeatureVector.h"
#include "WordsRange.h"
#include "Phrase.h"
#include "Factor.h"

using namespace Moses;

namespace Josiah {

class Sample;

/**
  * Represents a derivation, ie a way of getting from e to f.
  **/
  class Derivation {
    public:
      Derivation(const Sample& sample);
      void getTargetSentence(std::vector<std::string>&) const;
      int getTargetSentenceSize() const;
      const FVector& getFeatureValues() const {return m_featureValues;}
      float getScore() const {return m_score;}
      void getTargetFactors(std::vector<const Factor*>& sentence) const;
      bool operator<(const Derivation& other) const;
      
      struct PhraseAlignment {
        //since these are stored in target order, no need to retain the source Segment
        WordsRange _sourceSegment;
        Phrase _target;
        PhraseAlignment(const WordsRange& sourceSegment,const Phrase& target)
          : _sourceSegment(sourceSegment),_target(target) {}
        bool operator<(const PhraseAlignment& other) const;
      };
      
      friend std::ostream& operator<<(std::ostream&, const Derivation&);
      friend struct DerivationProbLessThan;
      
    private:
      std::vector<PhraseAlignment> m_alignments; //in target order
      FVector m_featureValues;
      FValue m_score;
      //std::vector<std::string> m_targetWords;
  };
  
  struct DerivationLessThan {
      bool operator()(const Derivation& d1, const Derivation& d2) {
      return d1 < d2; 
      }
  };
  
  typedef std::pair<const Derivation*,float> DerivationProbability;
  
  
  
  std::ostream& operator<<(std::ostream&, const Derivation&);
  
  

} //namespace


