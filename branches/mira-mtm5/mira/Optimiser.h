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
#ifndef _MIRA_OPTIMISER_H_
#define _MIRA_OPTIMISER_H_

#include <vector>

#include "ScoreComponentCollection.h"


namespace Mira {
  
  class Optimiser {
    public:
      virtual void updateWeights(const Moses::ScoreComponentCollection& currWeights,
                         const std::vector< std::vector<Moses::ScoreComponentCollection> >& scores,
                         const std::vector<float>& losses,
                         const Moses::ScoreComponentCollection oracleScores,
                         Moses::ScoreComponentCollection& newWeights) = 0;
  };
 
  class DummyOptimiser : public Optimiser {
    public:
      virtual void updateWeights(const Moses::ScoreComponentCollection& currWeights,
                         const std::vector< std::vector<Moses::ScoreComponentCollection> >& scores,
                         const std::vector<float>& losses,
                         const Moses::ScoreComponentCollection oracleScores,
                         Moses::ScoreComponentCollection& newWeights) {/* do nothing */}
  };

}

#endif
