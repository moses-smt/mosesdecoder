// $Id: LanguageModelIRST.h 3719 2010-11-17 14:06:21Z chardmeier $

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

#ifndef moses_LanguageModelIRST_h
#define moses_LanguageModelIRST_h

#include <string>
#include <vector>
#include "Factor.h"
#include "TypeDef.h"
#include "Util.h"
#include "LanguageModelSingleFactor.h"

class lmtable;  // irst lm table
class lmmacro;  // irst lm for macro tags
class ngram;

namespace Moses
{
class Phrase;
	
/** Implementation of single factor LM using IRST's code.
* This is the default LM for Moses and is available from the same sourceforge repository
*/
class LanguageModelIRST : public LanguageModelPointerState
{
protected:
	std::vector<int> m_lmIdLookup;
	lmtable* m_lmtb;
	ngram* m_lmtb_ng;
	
	int	m_unknownId;
	int m_lmtb_sentenceStart; //lmtb symbols to initialize ngram with
	int m_lmtb_sentenceEnd;   //lmt symbol to initialize ngram with 
	int m_lmtb_size;          //max ngram stored in the table
	int m_lmtb_dub;           //dictionary upperboud

	std::string m_mapFilePath;
  
//	float GetValue(LmId wordId, ngram *context) const;

	void CreateFactors(FactorCollection &factorCollection);
	int GetLmID( const std::string &str ) const;
	int GetLmID( const Factor *factor ) const;
  
public:
	LanguageModelIRST(int dub);
	~LanguageModelIRST();
	bool Load(const std::string &filePath
					, FactorType factorType
					, size_t nGramOrder);

  virtual float GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL, unsigned int* len=0) const;

  void CleanUpAfterSentenceProcessing();
  void InitializeBeforeSentenceProcessing();

  void set_dictionary_upperbound(int dub){ m_lmtb_size=dub ; 
//m_lmtb->set_dictionary_upperbound(dub);
};
};

}

#endif
