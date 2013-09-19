// $Id: Factory.cpp,v 1.1.1.1 2013/01/06 16:54:19 braunefe Exp $


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

#include <iostream>
#include "LM/Factory.h"
#include "UserMessage.h"
#include "TypeDef.h"
#include "FactorCollection.h"

/////////////////////////////////////////////////
// for those using autoconf/automake
#if HAVE_CONFIG_H
#include "config.h"

#define LM_REMOTE 1
#  ifdef HAVE_SRILM
#    define LM_SRI 1
#  else
#    undef LM_SRI
#  endif

#  ifdef HAVE_IRSTLM
#    define LM_IRST 1
#  endif

#  ifdef HAVE_RANDLM
#    define LM_RAND 1
#  endif

#  ifdef HAVE_ORLM
#    define LM_ORLM 1
#  endif

#  ifdef HAVE_DMAPLM
#    define LM_DMAP
#  endif
#endif

// include appropriate header
#ifdef LM_SRI
#  include "LM/SRI.h"
#include "LM/ParallelBackoff.h"
#endif
#ifdef LM_IRST
#  include "LM/IRST.h"
#endif
#ifdef LM_RAND
#  include "LM/Rand.h"
#endif
#ifdef LM_ORLM
#  include "LM/ORLM.h"
#endif
#ifdef LM_REMOTE
#	include "LM/Remote.h"
#endif
#include "LM/Ken.h"
#ifdef LM_DMAP
#   include "LM/DMapLM.h"
#endif

#include "LM/Base.h"
#include "LM/Joint.h"

using namespace std;

namespace Moses
{

namespace LanguageModelFactory
{

LanguageModel* CreateLanguageModel(LMImplementation lmImplementation
                                   , const std::vector<FactorType> &factorTypes
                                   , size_t nGramOrder
                                   , const std::string &languageModelFile
                                   , ScoreIndexManager &scoreIndexManager
                                   , int dub )
{
  if (lmImplementation == Ken || lmImplementation == LazyKen) {
    return ConstructKenLM(languageModelFile, scoreIndexManager, factorTypes[0], lmImplementation == LazyKen);
  }
  LanguageModelImplementation *lm = NULL;
  switch (lmImplementation) {
  case RandLM:
#ifdef LM_RAND
    lm = NewRandLM();
#endif
    break;
  case ORLM:
#ifdef LM_ORLM
    lm = new LanguageModelORLM();
#endif
    break;
  case Remote:
#ifdef LM_REMOTE
    lm = new LanguageModelRemote();
#endif
    break;

  case SRI:
#ifdef LM_SRI
    lm = new LanguageModelSRI();
#endif
    break;
  case IRST:
#ifdef LM_IRST
    lm = new LanguageModelIRST(dub);
#endif
    break;
  case Joint:
#ifdef LM_SRI
    lm = new LanguageModelJoint(new LanguageModelSRI());
#endif
    break;
  case ParallelBackoff:
#ifdef LM_SRI
    lm = NewParallelBackoff();
#endif
    break;
  case DMapLM:
#ifdef LM_DMAP
    lm = new LanguageModelDMapLM();
#endif
    break;
  default:
    break;
  }

  if (lm == NULL) {
    UserMessage::Add("Language model type unknown. Probably not compiled into library");
    return NULL;
  } else {
    switch (lm->GetLMType()) {
    case SingleFactor:
      if (! static_cast<LanguageModelSingleFactor*>(lm)->Load(languageModelFile, factorTypes[0], nGramOrder)) {
        cerr << "single factor model failed" << endl;
        delete lm;
        lm = NULL;
      }
      break;
    case MultiFactor:
      if (! static_cast<LanguageModelMultiFactor*>(lm)->Load(languageModelFile, factorTypes, nGramOrder)) {
        cerr << "multi factor model failed" << endl;
        delete lm;
        lm = NULL;
      }
      break;
    }
  }

  return new LMRefCount(scoreIndexManager, lm);
}
}

}

