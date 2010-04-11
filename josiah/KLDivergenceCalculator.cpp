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

#include "KLDivergenceCalculator.h"
#include "GibblerMaxTransDecoder.h"


namespace Josiah {

  KLDivergenceCalculator::KLDivergenceCalculator(const std::string& line) : m_input(line), m_Z(0.0) { 
    m_klDecoder.reset(new KLDecoder); 
    m_Z = m_klDecoder->GetZ(m_input); //calc sum of all paths in lattice
    cerr << "Lattice score for : "<< line << " = " << m_Z << endl;
  }
    
  float KLDivergenceCalculator::calcDDDivergence(const std::vector<std::pair<const Derivation*, float> >& nbest) {
    float kl = 0;
    for (size_t i = 0; i < nbest.size(); ++i) {
      float estimatedProb = log(nbest[i].second);
      float trueProb = (nbest[i].first)->getScore() - m_Z;
      IFVERBOSE(0) {
        cerr << *(nbest[i].first) << " ||| " << nbest[i].second << " ||| " << exp(trueProb) << endl;
      }
      kl += nbest[i].second * (estimatedProb - trueProb);
    }
    return kl;
  }
  
  
  float KLDivergenceCalculator::calcTDDivergence(const std::vector<std::pair<const Translation*, float> >& nbest) {
    float kl = 0;
    for (size_t i = 0; i < nbest.size(); ++i) {
      float estimatedProb = log(nbest[i].second);
      bool found = false;
      float trueProb = m_klDecoder->GetTranslationScore(*(nbest[i].first), found) - m_Z;
      IFVERBOSE(0) {
        cerr << *(nbest[i].first) << " ||| " << nbest[i].second << " ||| " << exp(trueProb) << endl;
        cerr << "KL Cont: " <<  nbest[i].second * (estimatedProb - trueProb) << endl;
      }
      if (found) {
        kl += nbest[i].second * (estimatedProb - trueProb);
      }
    }
    return kl;
  }

}
