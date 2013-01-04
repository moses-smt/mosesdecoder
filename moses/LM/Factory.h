// $Id$

#ifndef moses_LanguageModelFactory_h
#define moses_LanguageModelFactory_h

#include <string>
#include <vector>
#include "moses/TypeDef.h"

namespace Moses
{

class LanguageModel;

namespace LanguageModelFactory {

	/**
	 * creates a language model that will use the appropriate
   * language model toolkit as its underlying implementation
	 */
	 LanguageModel* CreateLanguageModel(LMImplementation lmImplementation
																		, const std::vector<FactorType> &factorTypes     
																		, size_t nGramOrder
																		, const std::string &languageModelFile
																		, int dub);
	 
};

}

#endif
