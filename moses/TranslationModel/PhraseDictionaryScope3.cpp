// vim:tabstop=2

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

#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include "PhraseDictionaryScope3.h"
#include "moses/FactorCollection.h"
#include "moses/Word.h"
#include "moses/Util.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/Range.h"
#include "moses/TranslationModel/RuleTable/LoaderFactory.h"
#include "moses/TranslationModel/RuleTable/Loader.h"
#include "moses/TranslationModel/Scope3Parser/Parser.h"
#include "moses/InputPath.h"

using namespace std;

namespace Moses
{
PhraseDictionaryScope3::PhraseDictionaryScope3(const std::string &line)
  : RuleTableUTrie(line)
{
  ReadParameters();

  // caching for memory pt is pointless
  m_maxCacheSize = 0;

}

}
