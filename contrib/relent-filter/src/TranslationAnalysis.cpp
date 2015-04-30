// $Id$

#include <iostream>
#include <sstream>
#include <algorithm>
#include "StaticData.h"
#include "Hypothesis.h"
#include "TranslationAnalysis.h"

using namespace Moses;

namespace TranslationAnalysis
{

void PrintTranslationAnalysis(const TranslationSystem* system, std::ostream &os, const Hypothesis* hypo)
{
  os << std::endl << "TRANSLATION HYPOTHESIS DETAILS:" << std::endl;
  std::vector<const Hypothesis*> translationPath;

  while (hypo) {
    translationPath.push_back(hypo);
    hypo = hypo->GetPrevHypo();
  }

  std::reverse(translationPath.begin(), translationPath.end());
  std::vector<std::string> droppedWords;
  std::vector<const Hypothesis*>::iterator tpi = translationPath.begin();
  if(tpi == translationPath.end())
    return;
  ++tpi;  // skip initial translation state
  std::vector<std::string> sourceMap;
  std::vector<std::string> targetMap;
  std::vector<unsigned int> lmAcc(0);
  size_t lmCalls = 0;
  bool doLMStats = ((*tpi)->GetLMStats() != 0);
  if (doLMStats)
    lmAcc.resize((*tpi)->GetLMStats()->size(), 0);
  for (; tpi != translationPath.end(); ++tpi) {
    std::ostringstream sms;
    std::ostringstream tms;
    std::string target = (*tpi)->GetTargetPhraseStringRep();
    std::string source = (*tpi)->GetSourcePhraseStringRep();
    WordsRange twr = (*tpi)->GetCurrTargetWordsRange();
    WordsRange swr = (*tpi)->GetCurrSourceWordsRange();
    const AlignmentInfo &alignmentInfo = (*tpi)->GetCurrTargetPhrase().GetAlignmentInfo();
    // language model backoff stats,
    if (doLMStats) {
      std::vector<std::vector<unsigned int> >& lmstats = *(*tpi)->GetLMStats();
      std::vector<std::vector<unsigned int> >::iterator i = lmstats.begin();
      std::vector<unsigned int>::iterator acc = lmAcc.begin();

      for (; i != lmstats.end(); ++i, ++acc) {
        std::vector<unsigned int>::iterator j = i->begin();
        lmCalls += i->size();
        for (; j != i->end(); ++j) {
          (*acc) += *j;
        }
      }
    }

    bool epsilon = false;
    if (target == "") {
      target="<EPSILON>";
      epsilon = true;
      droppedWords.push_back(source);
    }
    os	<< "         SOURCE: " << swr << " " << source << std::endl
        << "  TRANSLATED AS: "               << target << std::endl
        << "  WORD ALIGNED: " << alignmentInfo					<< std::endl;
    size_t twr_i = twr.GetStartPos();
    size_t swr_i = swr.GetStartPos();
    if (!epsilon) {
      sms << twr_i;
    }
    if (epsilon) {
      tms << "del(" << swr_i << ")";
    } else {
      tms << swr_i;
    }
    swr_i++;
    twr_i++;
    for (; twr_i <= twr.GetEndPos() && twr.GetEndPos() != NOT_FOUND; twr_i++) {
      sms << '-' << twr_i;
    }
    for (; swr_i <= swr.GetEndPos() && swr.GetEndPos() != NOT_FOUND; swr_i++) {
      tms << '-' << swr_i;
    }
    if (!epsilon) targetMap.push_back(sms.str());
    sourceMap.push_back(tms.str());
  }
  std::vector<std::string>::iterator si = sourceMap.begin();
  std::vector<std::string>::iterator ti = targetMap.begin();
  os << std::endl << "SOURCE/TARGET SPANS:";
  os << std::endl << "  SOURCE:";
  for (; si != sourceMap.end(); ++si) {
    os << " " << *si;
  }
  os << std::endl << "  TARGET:";
  for (; ti != targetMap.end(); ++ti) {
    os << " " << *ti;
  }
  os << std::endl << std::endl;
  if (doLMStats && lmCalls > 0) {
    std::vector<unsigned int>::iterator acc = lmAcc.begin();
    const LMList& lmlist = system->GetLanguageModels();
    LMList::const_iterator i = lmlist.begin();
    for (; acc != lmAcc.end(); ++acc, ++i) {
      char buf[256];
      sprintf(buf, "%.4f", (float)(*acc)/(float)lmCalls);
      os << (*i)->GetScoreProducerDescription() <<", AVG N-GRAM LENGTH: " << buf << std::endl;
    }
  }

  if (droppedWords.size() > 0) {
    std::vector<std::string>::iterator dwi = droppedWords.begin();
    os << std::endl << "WORDS/PHRASES DROPPED:" << std::endl;
    for (; dwi != droppedWords.end(); ++dwi) {
      os << "\tdropped=" << *dwi << std::endl;
    }
  }
  os << std::endl << "SCORES (UNWEIGHTED/WEIGHTED): ";
  StaticData::Instance().GetScoreIndexManager().PrintLabeledWeightedScores(os, translationPath.back()->GetScoreBreakdown(), StaticData::Instance().GetAllWeights());
  os << std::endl;
}

}
