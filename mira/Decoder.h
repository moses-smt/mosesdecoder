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
#ifndef _MIRA_DECODER_H_
#define _MIRA_DECODER_H_

#include <iostream>
#include <cstring>
#include <sstream>


#include "BleuScoreFeature.h"
#include "ChartTrellisPathList.h"
#include "Hypothesis.h"
#include "Parameter.h"
#include "SearchNormal.h"
#include "Sentence.h"
#include "StaticData.h"

//
// Wrapper functions and objects for the decoder.
//

namespace Mira {
  
/**
 * Initialise moses (including StaticData) using the given ini file and debuglevel, passing through any 
 * other command line arguments. 
 **/
void initMoses(const std::string& inifile, int debuglevel,  int argc=0, char** argv=NULL );


/**
  * Wraps moses decoder.
 **/
class MosesDecoder {
  public:
    MosesDecoder(const std::vector<std::vector<std::string> >& refs, bool useScaledReference, bool scaleByInputLength, float BPfactor, float historySmoothing);
	
    //returns the best sentence
    std::vector<const Moses::Word*> getNBest(const std::string& source,
                          size_t sentenceid,
                          size_t count,
                          float bleuObjectiveweight, //weight of bleu in objective
                          float bleuScoreWeight, //weight of bleu in score
                          std::vector< Moses::ScoreComponentCollection>& featureValues,
                          std::vector< float>& scores,
                          bool oracle,
                          bool distinct,
                          bool ignoreUWeight);
    size_t getCurrentInputLength();
    void updateHistory(const std::vector<const Moses::Word*>& words);
    void updateHistory(const std::vector< std::vector< const Moses::Word*> >& words, std::vector<size_t>& sourceLengths, std::vector<size_t>& ref_ids);
    Moses::ScoreComponentCollection getWeights();
    void setWeights(const Moses::ScoreComponentCollection& weights);
		void cleanup();
		
	private:
    float getBleuScore(const Moses::ScoreComponentCollection& scores);
    void setBleuScore(Moses::ScoreComponentCollection& scores, float bleu);
		Moses::Manager *m_manager;
		Moses::Sentence *m_sentence;
    Moses::BleuScoreFeature *m_bleuScoreFeature;
	

};


} //namespace

#endif
