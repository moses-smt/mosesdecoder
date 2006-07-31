// $Id$

#include "LanguageModelFactory.h"
#include "TypeDef.h"

// include appropriate header
#ifdef LM_SRI
#  include "LanguageModel_SRI.h"
#else
#  ifdef LM_IRST
#    include "LanguageModel_IRST.h"
#  else
#    include "LanguageModel_Internal.h"
#  endif
#endif


namespace LanguageModelFactory
{

	LanguageModel* createLanguageModel()
	{
	  LanguageModel *lm = 0;
#ifdef LM_SRI
	  lm = new LanguageModel_SRI();
#else
#  ifdef LM_IRST
	     lm = new LanguageModel_IRST();
#  else
	     lm = new LanguageModel_Internal();
#  endif
#endif
	  return lm;
	}

}
