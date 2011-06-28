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

#pragma once

#include "StatelessFeature.h"

namespace Josiah {

/**
  * The 'phrase pair' features of Watanabe et al. These are formed by 
  * pairing aligned words (or other factors) between the source and target 
  * side of a phrase pair.
 **/
class PhrasePairFeature : public StatelessFeature {
  public:
    static const std::string PREFIX;

    PhrasePairFeature(Moses::FactorType sourceFactorId,
                      Moses::FactorType targetFactorId);

    const Moses::Factor* getSourceFactor(const Moses::Word& word) const;
    const Moses::Factor* getTargetFactor(const Moses::Word& word) const;
    /** Scores due to this translation option */
    virtual void assign
      (const Moses::TranslationOption* option, FVector& scores) const;
    
  private:
    Moses::FactorType m_sourceFactorId;
    Moses::FactorType m_targetFactorId; 

};
}

