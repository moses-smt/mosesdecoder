#include <iostream>
#include <sstream>
#include <algorithm>
#include "Hypothesis.h"
#include "TranslationAnalysis.h"

namespace TranslationAnalysis {

void PrintTranslationAnalysis(std::ostream &os, const Hypothesis* hypo)
{
  std::vector<const Hypothesis*> translationPath;
  while (hypo) {
    translationPath.push_back(hypo);
    hypo = hypo->GetPrevHypo();
  }
  std::reverse(translationPath.begin(), translationPath.end());

  std::vector<std::string> droppedWords;
  std::vector<const Hypothesis*>::iterator tpi = translationPath.begin();
  ++tpi;  // skip initial translation state
	std::vector<std::string> sourceMap;
	std::vector<std::string> targetMap;
  for (; tpi != translationPath.end(); ++tpi) {
		std::ostringstream sms;
		std::ostringstream tms;
    std::string target = (*tpi)->GetTargetPhraseStringRep();
    std::string source = (*tpi)->GetSourcePhraseStringRep();
		WordsRange twr = (*tpi)->GetCurrTargetWordsRange();
		WordsRange swr = (*tpi)->GetCurrSourceWordsRange();
		bool epsilon = false;
    if (target == "") {
      target="<EPSILON>";
			epsilon = true;
      droppedWords.push_back(source);
    }
    os << "       SOURCE: " << swr << " " << source << std::endl
       << "TRANSLATED AS: "               << target << std::endl;
		size_t twr_i = twr.GetStartPos();
		size_t swr_i = swr.GetStartPos();
		if (!epsilon) { sms << twr_i; }
		if (epsilon) { tms << "del(" << swr_i << ")"; } else { tms << swr_i; }
		swr_i++; twr_i++;
		for (; twr_i <= twr.GetEndPos(); twr_i++) {
			sms << '-' << twr_i;
		}
		for (; swr_i <= swr.GetEndPos(); swr_i++) {
			tms << '-' << swr_i;
		}
		if (!epsilon) targetMap.push_back(sms.str());
		sourceMap.push_back(tms.str());
  }
	std::vector<std::string>::iterator si = sourceMap.begin();
	std::vector<std::string>::iterator ti = targetMap.begin();
	os << "SOURCE:";
	for (; si != sourceMap.end(); ++si) {
		os << " " << *si;
	}
	os << std::endl << "TARGET:";
	for (; ti != targetMap.end(); ++ti) {
		os << " " << *ti;
	}
	os << std::endl;

  if (droppedWords.size() > 0) {
    std::vector<std::string>::iterator dwi = droppedWords.begin();
    os << "WORDS/PHRASES DROPPED:" << std::endl;
    for (; dwi != droppedWords.end(); ++dwi) {
      os << "\t" << *dwi << std::endl;
    }
  }
}

}
