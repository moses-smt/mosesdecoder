/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#include "PhraseDictionaryNodeMemory.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/InputType.h"
#include "moses/NonTerminal.h"
#include "moses/TranslationModel/RuleTable/UTrie.h"

namespace Moses
{
class ChartParser;

/** Scope-3 only
 */
class PhraseDictionaryScope3 : public RuleTableUTrie
{
  friend class RuleTableLoader;

public:
  PhraseDictionaryScope3(const std::string &line);

protected:

};

}  // namespace Moses
