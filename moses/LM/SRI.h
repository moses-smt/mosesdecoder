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

#ifndef moses_LanguageModelSRI_h
#define moses_LanguageModelSRI_h

#include <string>
#include <vector>
#include "moses/Factor.h"
#include "moses/TypeDef.h"
#include "SingleFactor.h"

class Factor;
class Phrase;
class Vocab;
class Ngram;

namespace Moses
{

/** Implementation of single factor LM using IRST's code.
 */
class LanguageModelSRI : public LanguageModelSingleFactor
{
protected:
  std::vector<unsigned int> m_lmIdLookup;
  ::Vocab			*m_srilmVocab;
  Ngram 			*m_srilmModel;
  unsigned int	m_unknownId;

  LMResult GetValue(unsigned int wordId, unsigned int *context) const;
  void CreateFactors();
  unsigned int GetLmID( const std::string &str ) const;
  unsigned int GetLmID( const Factor *factor ) const;

public:
  LanguageModelSRI(const std::string &line);
  ~LanguageModelSRI();
  void Load(AllOptions::ptr const& opts);

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;
};


}
#endif
