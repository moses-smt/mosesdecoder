// $Id$

#include <iostream>
#include <sstream>
#include <algorithm>

#include "TranslationAnalysis.h"

#include "moses/StaticData.h"
#include "moses/TranslationOption.h"
#include "moses/DecodeStepTranslation.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/LM/Base.h"

using namespace std;
using namespace Moses;

namespace TranslationAnalysis
{

void PrintTranslationAnalysis(ostream & /* os */, const Hypothesis* /* hypo */)
{
  /*
  os << endl << "TRANSLATION HYPOTHESIS DETAILS:" << endl;
  queue<const Hypothesis*> translationPath;
  while (hypo)
  {
  	translationPath.push(hypo);
    hypo = hypo->GetPrevHypo();
  }

  while (!translationPath.empty())
  {
  	hypo = translationPath.front();
  	translationPath.pop();
  	const TranslationOption *transOpt = hypo->GetTranslationOption();
  	if (transOpt != NULL)
  	{
  		os	<< hypo->GetCurrSourceWordsRange() << "  ";
  		for (size_t decodeStepId = 0; decodeStepId < DecodeStepTranslation::GetNumTransStep(); ++decodeStepId)
  			os << decodeStepId << "=" << transOpt->GetSubRangeCount(decodeStepId) << ",";
  		os	<< *transOpt << endl;
  	}
  }

  os << "END TRANSLATION" << endl;
  */
}

}

