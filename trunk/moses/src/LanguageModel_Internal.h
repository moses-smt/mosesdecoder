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

class LanguageModel_Internal : public LanguageModel
{
public:
	
	LanguageModel_Internal();
	void Load(const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder);
	
#define Vocab_None NULL;

  virtual float GetValue(const std::vector<const Factor*> &contextFactor, State* finalState = 0) const;

protected:
	NGramCollection m_map;

	float GetValue(const Factor *factor0) const;
	float GetValue(const Factor *factor0, const Factor *factor1) const;
	float GetValue(const Factor *factor0, const Factor *factor1, const Factor *factor2) const;

public:
	LmId GetLmID( const Factor *factor )  const;

};

