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
#include "TypeDef.h"
#include "Util.h"
#include "LanguageModelSingleFactor.h"
#include "../../kenlm/lm/ngram.hh"

namespace Moses
{
class Phrase;
	
/** Implementation of single factor LM using Ken's code.
*/
class LanguageModelKen : public LanguageModelSingleFactor
{
protected:
  lm::ngram::Model *m_ngram;
	
public:
	LanguageModelKen(bool registerScore, ScoreIndexManager &scoreIndexManager, int dub);
	~LanguageModelKen();
	bool Load(const std::string &filePath
					, FactorType factorType
					, size_t nGramOrder);

  virtual float GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL, unsigned int* len=0) const;

  void CleanUpAfterSentenceProcessing() {}
  void InitializeBeforeSentenceProcessing() {}

};
};


#endif
