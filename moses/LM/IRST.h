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

#ifndef moses_LanguageModelIRST_h
#define moses_LanguageModelIRST_h

#include <string>
#include <vector>
#include "moses/Factor.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "SingleFactor.h"

class lmContainer;  // irst lm container for any lm type
class ngram;
class dictionary;

namespace Moses
{
class Phrase;

/** Implementation of single factor LM using IRST's code.
 * This is available from the same sourceforge repository
 */
class LanguageModelIRST : public LanguageModelSingleFactor
{
protected:
  mutable std::vector<int> m_lmIdLookup;
  lmContainer* m_lmtb;

  int m_unknownId;  //code of OOV
  int m_empty;  //code of an empty position
  int m_lmtb_sentenceStart; //lmtb symbols to initialize ngram with
  int m_lmtb_sentenceEnd;   //lmt symbol to initialize ngram with
  int m_lmtb_size;          //max ngram stored in the table
  int m_lmtb_dub;           //dictionary upperboud

  std::string m_mapFilePath;

  void CreateFactors(FactorCollection &factorCollection);
  int GetLmID( const std::string &str ) const;
  int GetLmID( const Factor *factor ) const;

  dictionary* d;

public:
  LanguageModelIRST(const std::string &line);

  ~LanguageModelIRST();
  void Load();

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL) const;

  void InitializeForInput(InputType const& source);
  void CleanUpAfterSentenceProcessing(const InputType& source);

  void set_dictionary_upperbound(int dub) {
    m_lmtb_size=dub ;
//m_lmtb->set_dictionary_upperbound(dub);
  };
};

}

#endif
