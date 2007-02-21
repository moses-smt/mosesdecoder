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

#include "LanguageModelInternal.h"
#include "LanguageModelSkip.h"
#include "LanguageModelJoint.h"

namespace LanguageModelFactory
{

	LanguageModel* CreateLanguageModel(LMImplementation lmImplementation
																		, const std::vector<FactorType> &factorTypes     
																		, size_t nGramOrder
																		, const std::string &languageModelFile
																		, float weight
																		, ScoreIndexManager &scoreIndexManager)
	{
	  LanguageModel *lm = NULL;
	  switch (lmImplementation)
	  {
	  	case SRI:
				#ifdef LM_SRI
				  lm = new LanguageModelSRI(true, scoreIndexManager);
				#elif LM_INTERNAL
					lm = new LanguageModelInternal(true, scoreIndexManager);
				#elif LM_IRST
					// shouldn't really do this. the 2 lm are not compatible
					lm = new LanguageModelIRST(true, scoreIndexManager);
			  #endif
			  break;
			case IRST:
				#ifdef LM_IRST
	     		lm = new LanguageModelIRST(true, scoreIndexManager);
				#elif LM_SRI
					// shouldn't really do this. the 2 lm are not compatible
				  lm = new LanguageModelSRI(true, scoreIndexManager);
				#elif LM_INTERNAL
					// shouldn't really do this. the 2 lm are not compatible
					lm = new LanguageModelInternal(true, scoreIndexManager);
			  #endif
				break;
			case Skip:
				#ifdef LM_SRI
	     		lm = new LanguageModelSkip(new LanguageModelSRI(false, scoreIndexManager)
																		, true
																		, scoreIndexManager);
				#elif LM_INTERNAL
     			lm = new LanguageModelSkip(new LanguageModelInternal(false, scoreIndexManager)
																		, true
																		, scoreIndexManager);
				#elif LM_IRST
					// shouldn't really do this. the 2 lm are not compatible
	     		lm = new LanguageModelSkip(new LanguageModelIRST(false, scoreIndexManager)
																		, true
																		, scoreIndexManager);
				#endif
				break;
			case Joint:
				#ifdef LM_SRI
	     		lm = new LanguageModelJoint(new LanguageModelSRI(false), true);
				#elif LM_INTERNAL
	     		lm = new LanguageModelJoint(new LanguageModelInternal(false, scoreIndexManager)
																		, true
																		, scoreIndexManager);
				#elif LM_IRST
					// shouldn't really do this. the 2 lm are not compatible
	     		lm = new LanguageModelJoint(new LanguageModelIRST(false), true);
				#endif
				break;
	  	case Internal:
				#ifdef LM_INTERNAL
					lm = new LanguageModelInternal(true, scoreIndexManager);
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
	  		if (! static_cast<LanguageModelSingleFactor*>(lm)->Load(languageModelFile, factorTypes[0], weight, nGramOrder))
				{
					delete lm;
					lm = NULL;
				}
	  		break;	  	
	  	case MultiFactor:
  			if (! static_cast<LanguageModelMultiFactor*>(lm)->Load(languageModelFile, factorTypes, weight, nGramOrder))
				{
					delete lm;
					lm = NULL;
				}
  			break;
	  	}
	  }
	  
	  return lm;
	}
}
