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

#include "Gibbler.h"
#include "ScoreComponentCollection.h"
#include "WordsRange.h"

namespace Josiah {

/**
  * Represents a derivation, ie a way of getting from e to f.
  **/
  class Derivation {
    public:
      Derivation(const Moses::Sample& sample);
      const std::vector<std::string>& getTargetSentence() const {return m_targetWords;}
      const Moses::ScoreComponentCollection& getFeatureValues() const {return m_featureValues;}
      float getScore() const {return m_score;}
      bool operator<(const Derivation& other) const;
      
      struct PhraseAlignment {
        //since these are stored in target order, no need to retain the source Segment
        Moses::WordsRange _sourceSegment;
        Moses::Phrase _target;
        PhraseAlignment(const Moses::WordsRange& sourceSegment,const Moses::Phrase& target)
          : _sourceSegment(sourceSegment),_target(target) {}
        bool operator<(const PhraseAlignment& other) const;
      };
      
      friend std::ostream& operator<<(std::ostream&, const Derivation&);
      friend struct DerivationProbLessThan;
      
    private:
      std::vector<PhraseAlignment> m_alignments; //in target order
      Moses::ScoreComponentCollection m_featureValues;
      float m_score;
      std::vector<std::string> m_targetWords;
  };
  
  struct DerivationLessThan {
      bool operator()(const Derivation& d1, const Derivation& d2) {
      return d1 < d2; 
      }
  };
  
  typedef std::pair<const Derivation*,float> DerivationProbability;
  
  struct DerivationProbGreaterThan :  public std::binary_function<const DerivationProbability,const DerivationProbability,bool>{
    bool operator()(const DerivationProbability& d1, const DerivationProbability& d2) const {
        return d1.second > d2.second; 
       }
  };
  
  std::ostream& operator<<(std::ostream&, const Derivation&);

  class DerivationCollector: public virtual Moses::SampleCollector {
    public:
      DerivationCollector(): m_n(0),m_pd(0) ,m_collectDerivByTrans(false), m_outputMaxChange(false), m_maxDerivation(NULL) {}
      void collect(Moses::Sample& sample);
      /** Top n in descending order. */
      void getTopN(size_t n, std::vector<DerivationProbability>& derivations);
      /** Write max periodically to stderr */
      void setPeriodicDecode(int pd) {m_pd = pd;}
      /** Write the max derivation every time it changes */
      void setOutputMaxChange(bool outputMaxChange) {m_outputMaxChange = outputMaxChange;}
      void setCollectDerivationsByTranslation(bool dbyt) {m_collectDerivByTrans = dbyt;}
      void outputDerivationsByTranslation(std::ostream& out);
      void outputDerivationProbability(const DerivationProbability& dp, std::ostream& out);
      
    private:
      std::map<Derivation,size_t> m_counts;
      std::map<std::string,std::set<Derivation> > m_derivByTrans;
      size_t m_n;
      int m_pd;
      bool m_collectDerivByTrans;
      bool m_outputMaxChange;
      const Derivation* m_maxDerivation;
  };

} //namespace


