// $Id$

#ifndef _LANGUAGE_MODEL_FACTORY_H_
#define _LANGUAGE_MODEL_FACTORY_H_

#include "TypeDef.h"

class LanguageModelSingleFactor;

namespace LanguageModelFactory {

	/**
	 * creates a language model that will use the appropriate
   * language model toolkit as its underlying implementation
	 */
	LanguageModelSingleFactor* createLanguageModelSingleFactor(LMType lmType);

};

#endif
