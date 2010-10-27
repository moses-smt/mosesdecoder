// $Id$

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

#ifndef moses_LanguageModelKen_h
#define moses_LanguageModelKen_h

#include <string>
#include <vector>
#include "Factor.h"
#include "FFState.h"
#include "TypeDef.h"
#include "Util.h"
#include "LanguageModelSingleFactor.h"
#include "lm/ngram.hh"

namespace Moses
{
class Phrase;
	
/** Implementation of single factor LM using Ken's code.
*/
class LanguageModelKen : public LanguageModelSingleFactor
{
private:
	lm::ngram::Model *m_ngram;
	std::vector<lm::WordIndex> m_lmIdLookup;

	void TranslateIDs(const std::vector<const Word*> &contextFactor, lm::WordIndex *indices) const;
	
public:
	LanguageModelKen(bool registerScore, ScoreIndexManager &scoreIndexManager);
	~LanguageModelKen();
	bool Load(const std::string &filePath
					, FactorType factorType
					, size_t nGramOrder);

	float GetValueGivenState(const std::vector<const Word*> &contextFactor, FFState &state, unsigned int* len = 0) const;
	float GetValueForgotState(const std::vector<const Word*> &contextFactor, FFState &outState, unsigned int* len=0) const;
	void GetState(const std::vector<const Word*> &contextFactor, FFState &outState) const;

	FFState *NewState(const FFState *from = NULL) const;

	lm::WordIndex GetLmID(const std::string &str) const;

	void CleanUpAfterSentenceProcessing() {}
	void InitializeBeforeSentenceProcessing() {}

};
};


#endif
