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

#ifndef moses_Dictionary_h
#define moses_Dictionary_h

#include <vector>
#include "FactorTypeSet.h"
#include "ScoreProducer.h"

namespace Moses
{
class InputType;
  
/** Abstract class from which PhraseDictionary and GenerationDictionary are inherited.
 */
class Dictionary
{
protected:

  const size_t m_numScoreComponent;

public:
  //! Constructor
  Dictionary(size_t numScoreComponent);
  //!Destructor
  virtual ~Dictionary();


  //! returns whether this dictionary is to be used for Translate or Generate
  virtual DecodeType GetDecodeType() const = 0;

  // clean up temporary memory, called after processing each sentence
  virtual void CleanUp(const InputType& source);
};

}
#endif
