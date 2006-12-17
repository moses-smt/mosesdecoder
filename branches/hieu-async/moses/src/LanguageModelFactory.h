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

#ifndef _LANGUAGE_MODEL_FACTORY_H_
#define _LANGUAGE_MODEL_FACTORY_H_

#include <string>
#include <vector>
#include "TypeDef.h"

class LanguageModel;
class FactorCollection;
class ScoreIndexManager;

namespace LanguageModelFactory {

	/**
	 * creates a language model that will use the appropriate
   * language model toolkit as its underlying implementation
	 */
	 LanguageModel* CreateLanguageModel(LMImplementation lmImplementation
																		, const std::vector<FactorType> &factorTypes     
																		, size_t nGramOrder
																		, const std::string &languageModelFile
																		, float weight
																		, FactorCollection &factorCollection
																		, ScoreIndexManager &scoreIndexManager);
	 
};

#endif
