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

#pragma once

#include <string>
#include <vector>
#include "NGramNode.h"
#include "Factor.h"
#include "TypeDef.h"
#include "Util.h"

#include "LanguageModel.h"


class FactorCollection;
class Factor;
class Phrase;

#include "Vocab.h"
class Vocab; // SRI forward decl
class Ngram; // SRI forward decl

class LanguageModel_SRI : public LanguageModel
{
public:
	
	LanguageModel_SRI();
	~LanguageModel_SRI();
	void Load(size_t id
					, const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder);

private:
  // not thread-safe, but not much is
	mutable VocabIndex m_context[MAX_NGRAM_SIZE];	

protected:
	Vocab *m_srilmVocab;
	Ngram *m_srilmModel;

	float GetValue(VocabIndex wordId, VocabIndex *context) const;

	void CreateFactors(FactorCollection &factorCollection);
public:
	LmId GetLmID( const std::string &str ) const;

  virtual float GetValue(const vector<const Factor*> &contextFactor) const;
};

