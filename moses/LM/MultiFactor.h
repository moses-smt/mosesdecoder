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

#ifndef moses_LanguageModelMultiFactor_h
#define moses_LanguageModelMultiFactor_h

#include <vector>
#include <string>
#include "Implementation.h"
#include "moses/Word.h"
#include "moses/FactorTypeSet.h"

namespace Moses
{

class Phrase;

/* Abstract class for for multi factor LM. Only inherited by the JointLM at the moment.
 * Could use this when factored LM are implemented
 */
class LanguageModelMultiFactor : public LanguageModelImplementation
{
protected:
  FactorMask m_factorTypes;

  LanguageModelMultiFactor(const std::string &line)
    :LanguageModelImplementation(line) {
  }

public:
  virtual bool Load(const std::string &filePath
                    , const std::vector<FactorType> &factorTypes
                    , size_t nGramOrder) = 0;

};

}
#endif
