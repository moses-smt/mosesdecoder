// $Id: TranslationAnalysis.h 2939 2010-02-24 11:15:44Z jfouet $

/*
 * also see moses/SentenceStats
 */

#ifndef moses_cmd_TranslationAnalysis_h
#define moses_cmd_TranslationAnalysis_h

#include <iostream>
#include "Hypothesis.h"

namespace TranslationAnalysis
{

/***
 * print details about the translation represented in hypothesis to
 * os.  Included information: phrase alignment, words dropped, scores
 */
	void PrintTranslationAnalysis(std::ostream &os, const Moses::Hypothesis* hypo);

}

#endif
