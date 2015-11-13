#pragma once
// $Id$

/*
 * also see moses/SentenceStats
 */

#include <iostream>

namespace Moses
{
class Hypothesis;
class ChartHypothesis;
}

namespace TranslationAnalysis
{

/***
 * print details about the translation represented in hypothesis to
 * os.  Included information: phrase alignment, words dropped, scores
 */
void PrintTranslationAnalysis(std::ostream &os, const Moses::Hypothesis* hypo);
void PrintTranslationAnalysis(std::ostream &os, const Moses::ChartHypothesis* hypo);

}

