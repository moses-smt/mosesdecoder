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
