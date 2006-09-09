// $Id$

#include "LanguageModelFactory.h"
#include "UserMessage.h"
#include "TypeDef.h"
#include "FactorCollection.h"

// include appropriate header
#ifdef LM_SRI
#  include "LanguageModelSRI.h"
#endif
#ifdef LM_IRST
#  include "LanguageModelIRST.h"
#endif

#include "LanguageModelChunking.h"
#include "LanguageModelJoint.h"

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
				  lm = new LanguageModelSRI(true);
			  #else
	     		lm = new LanguageModelIRST(true);
			  #endif
			  break;
			case IRST:
				#ifdef LM_IRST
	     		lm = new LanguageModelIRST(true);
			  #else
				  lm = new LanguageModelSRI(true);
				#endif
				break;
			case Chunking:
				#ifdef LM_SRI
	     		lm = new LanguageModelChunking(new LanguageModelSRI(false), true);
				#else
     			#ifdef LM_IRST
	     			lm = new LanguageModelChunking(new LanguageModelIRST(false), true);
     			#endif
				#endif
				break;
			case Joint:
				#ifdef LM_SRI
	     		lm = new LanguageModelJoint(new LanguageModelSRI(false), true);
				#else
     			#ifdef LM_IRST
		     		lm = new LanguageModelJoint(new LanguageModelIRST(false), true);
     			#endif
				#endif
				break;
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
