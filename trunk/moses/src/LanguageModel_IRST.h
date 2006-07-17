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
#include "Factor.h"
#include "TypeDef.h"
#include "Util.h"

#include "LanguageModel.h"

class FactorCollection;
class Factor;
class Phrase;

class lmtable;  // irst lm table
class ngram;

class LanguageModel_IRST : public LanguageModel
{
public:
	
	LanguageModel_IRST();
	~LanguageModel_IRST();
	void Load(size_t id
					, const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder);
	
protected:
	lmtable* m_lmtb;

//	float GetValue(LmId wordId, ngram *context) const;

	void CreateFactors(FactorCollection &factorCollection);
public:
	LmId GetLmID( const std::string &str ) const;
  virtual float GetValue(const std::vector<const Factor*> &contextFactor) const;

};

