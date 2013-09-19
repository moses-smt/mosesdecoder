// $Id: TranslationAnalysis.h,v 1.1.1.1 2013/01/06 16:54:09 braunefe Exp $

/*
 * also see moses/SentenceStats
 */

#ifndef _TRANSLATION_ANALYSIS_H_
#define _TRANSLATION_ANALYSIS_H_

#include <iostream>
#include "ChartHypothesis.h"

namespace TranslationAnalysis
{

/***
 * print details about the translation represented in hypothesis to
 * os.  Included information: phrase alignment, words dropped, scores
 */
void PrintTranslationAnalysis(std::ostream &os, const Moses::Hypothesis* hypo);

}

#endif
