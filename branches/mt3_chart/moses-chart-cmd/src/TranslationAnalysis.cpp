// $Id: TranslationAnalysis.cpp 552 2009-01-09 14:05:34Z hieu $

#include <iostream>
#include <sstream>
#include <algorithm>
#include "StaticData.h"
#include "TranslationAnalysis.h"
#include "TranslationOption.h"
#include "DecodeStepTranslation.h"
#include "../../moses-chart/src/ChartHypothesis.h"

using namespace std;
using namespace MosesChart;

namespace TranslationAnalysis {

void PrintTranslationAnalysis(ostream &os, const Hypothesis* hypo)
{
  if (hypo == NULL)
    return;
    
  os << " * " << hypo->GetId() << " " << hypo->GetCurrSourceRange() << " " << hypo->GetCurrTargetPhrase() << endl;
  
  const vector<const Hypothesis*> &prevHypos = hypo->GetPrevHypos();
  vector<const Hypothesis*>::const_iterator iter;
  for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter)
  {
    const Hypothesis *prevHypo = *iter;
    PrintTranslationAnalysis(os, prevHypo);
  }
}

}

