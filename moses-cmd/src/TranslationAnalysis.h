// $Id$

/*
 * also see moses/SentenceStats
 */

#ifndef moses_cmd_TranslationAnalysis_h
#define moses_cmd_TranslationAnalysis_h

#include <iostream>
#include "Hypothesis.h"
#include "TranslationSystem.h"

namespace TranslationAnalysis
{

/***
 * print details about the translation represented in hypothesis to
 * os.  Included information: phrase alignment, words dropped, scores
 */
void PrintTranslationAnalysis(const Moses::TranslationSystem* system, std::ostream &os, const Moses::Hypothesis* hypo);

}

#endif
