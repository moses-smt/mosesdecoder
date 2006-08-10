// $Id$

#include "LanguageModelFactory.h"
#include "UserMessage.h"
#include "TypeDef.h"
#include "FactorCollection.h"

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

	LanguageModel* CreateLanguageModel(LMImplementation lmImplementation, const std::vector<FactorType> &factorTypes     
                                   , size_t nGramOrder, const std::string &languageModelFile, float weight, FactorCollection &factorCollection)
	{
	  LanguageModel *lm = NULL;
	  switch (lmImplementation)
	  {
	  	case SRI:
				#ifdef LM_SRI
				  lm = new LanguageModel_SRI(true);
			  #else
	     		lm = new LanguageModel_IRST(true);
			  #endif
			  break;
			case IRST:
				#ifdef LM_IRST
	     		lm = new LanguageModel_IRST(true);
			  #else
				  lm = new LanguageModel_SRI(true);
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
	  
	  if (lm == NULL)
	  {
	  	UserMessage::Add("Language model type unknown. Probably not compiled into library");
	  }
	  else
	  {
	  	switch (lm->GetLMType())
	  	{
	  	case SingleFactor:
	  		static_cast<LanguageModelSingleFactor*>(lm)->Load(languageModelFile, factorCollection, factorTypes[0], weight, nGramOrder);
	  		break;	  	
	  	case MultiFactor:
	  		 static_cast<LanguageModelMultiFactor*>(lm)->Load(languageModelFile, factorCollection, factorTypes, weight, nGramOrder);
	  			break;
	  	}
	  }
	  
	  return lm;
	}
}
