// $Id$

#ifndef _LANGUAGE_MODEL_FACTORY_H_
#define _LANGUAGE_MODEL_FACTORY_H_

#include "TypeDef.h"

class LanguageModel;

namespace LanguageModelFactory {

	/**
	 * creates a language model that will use the appropriate
   * language model toolkit as its underlying implementation
	 */
	LanguageModel* createLanguageModel(LMType lmType);

};

#endif
