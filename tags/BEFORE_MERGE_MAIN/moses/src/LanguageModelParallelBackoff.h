// $Id: LanguageModelJoint.h 1897 2008-10-08 23:51:26Z hieuhoang1972 $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#include <vector>
#include <string>
#include <sstream>
#include <fstream>

#include "LanguageModelMultiFactor.h"
#include "Word.h"
#include "Factor.h"
#include "FactorTypeSet.h"
#include "FactorCollection.h"
#include "Phrase.h"

#include "FNgramSpecs.h"
#include "FNgramStats.h"
#include "FactoredVocab.h"
#include "FNgram.h"
#include "wmatrix.h"
#include "Vocab.h"




using namespace std;

//class FactoredVocab;
//class FNgram;
//class WidMatrix;


namespace Moses
{




/** LM of multiple factors. A simple extension of single factor LM - factors backoff together.
 *	Rather slow as this uses string concatenation/split
*/
class LanguageModelParallelBackoff : public LanguageModelMultiFactor
{
protected:
	std::vector<FactorType> m_factorTypesOrdered;

	FactoredVocab		*m_srilmVocab;
	FNgram 					*m_srilmModel;
	VocabIndex	m_unknownId;
	VocabIndex  m_wtid;
	VocabIndex  m_wtbid;
	VocabIndex  m_wteid;
  FNgramSpecs<FNgramCount>* fnSpecs;
	//std::vector<VocabIndex> m_lmIdLookup;
	std::map<size_t, VocabIndex>* lmIdMap;
	std::fstream* debugStream;

  WidMatrix *widMatrix;

public:
	LanguageModelParallelBackoff(bool registerScore, ScoreIndexManager &scoreIndexManager);

	
	~LanguageModelParallelBackoff();

	bool Load(const std::string &filePath, const std::vector<FactorType> &factorTypes, float weight, size_t nGramOrder);

VocabIndex GetLmID( const std::string &str ) const;

VocabIndex GetLmID( const Factor *factor, FactorType ft ) const;

void CreateFactors();
	
virtual float GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0, unsigned int* len = 0) const;

	
};

}
