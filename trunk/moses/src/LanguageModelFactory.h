// $Id$

#ifndef _LANGUAGE_MODEL_FACTORY_H_
#define _LANGUAGE_MODEL_FACTORY_H_

#include <string>
#include <vector>
#include "TypeDef.h"

class LanguageModel;

namespace LanguageModelFactory {

	/**
	 * creates a language model that will use the appropriate
   * language model toolkit as its underlying implementation
	 */
	 LanguageModel* CreateLanguageModel(LMImplementation lmImplementation, const std::vector<FactorType> &factorTypes     
                                   , size_t nGramOrder, const std::string &languageModelFile, float weight);
	 
};

#endif
