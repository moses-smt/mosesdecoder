// $Id$

#include "LanguageModelFactory.h"
#include "UserMessage.h"
#include "TypeDef.h"

// include appropriate header
#ifdef LM_SRI
#  include "LanguageModel_SRI.h"
#endif
#ifdef LM_IRST
#  include "LanguageModel_IRST.h"
#endif

#include "LanguageModel_Chunking.h"

namespace LanguageModelFactory
{

	LanguageModel* CreateLanguageModel(LMType lmType)
	{
	  LanguageModel *lm = NULL;
	  switch (lmType)
	  {
	  	case SRI:
				#ifdef LM_SRI
				  lm = new LanguageModel_SRI(true);
			  #endif
			  break;
			case IRST:
				#ifdef LM_IRST
	     		lm = new LanguageModel_IRST(true);
				#endif
				break;
			case Chunking:
				#ifdef LM_SRI
	     		lm = new LanguageModel_Chunking<LanguageModel_SRI>(true);
				#else
     			#ifdef LM_IRST
	     			lm = new LanguageModel_Chunking<LanguageModel_IRST>(true);
     			#endif
				#endif
	  }
	  
  	// fall back. pick what we have. assume it single factor
	  // we should be taking this out soon
	  if (lm == NULL)
	  {
	  	#ifdef LM_SRI
				lm = new LanguageModel_SRI(true);
			#else
				#ifdef LM_IRST
					lm = new LanguageModel_IRST(true);
				#else
			  	UserMessage::Add("Language model type unknown. Probably not compiled into library");
				#endif
			#endif			  
	  }
	  return lm;
	}
}
