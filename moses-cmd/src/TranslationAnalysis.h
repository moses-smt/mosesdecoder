#ifndef _TRANSLATION_ANALYSIS_H_
#define _TRANSLATION_ANALYSIS_H_

#include <iostream>

class Hypothesis;

namespace TranslationAnalysis
{

/***
 * print details about the translation represented in hypothesis to
 * os.  Included information: phrase alignment, words dropped, scores
 */
void PrintTranslationAnalysis(std::ostream &os, const Hypothesis* hypo);

}

#endif
