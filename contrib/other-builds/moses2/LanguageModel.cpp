/*
 * LanguageModel.cpp
 *
 *  Created on: 29 Oct 2015
 *      Author: hieu
 */

#include "LanguageModel.h"
#include "System.h"
#include "moses/Util.h"
#include "moses/InputFileStream.h"

using namespace std;

LanguageModel::LanguageModel(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
	ReadParameters();
}

LanguageModel::~LanguageModel() {
	// TODO Auto-generated destructor stub
}

void LanguageModel::Load(System &system)
{
  Moses::FactorCollection &fc = system.GetVocab();

  Moses::InputFileStream infile(m_path);
  size_t lineNum = 0;
  string line;
  while (getline(infile, line)) {
	  if (++lineNum % 10000 == 0) {
		  cerr << lineNum << " ";
	  }

	  vector<string> substrings;
	  Moses::Tokenize(substrings, line, "\t");

	  if (substrings.size() < 2)
		   continue;

	  assert(substrings.size() == 2 || substrings.size() == 3);

	  SCORE prob = Moses::Scan<SCORE>(substrings[0]);
	  if (substrings[1] == "<unk>") {
		  m_oov = prob;
		  continue;
	  }

	  SCORE backoff = 0.f;
	  if (substrings.size() == 3)
		backoff = Moses::Scan<SCORE>(substrings[2]);

	  // ngram
	  vector<string> key;
	  Moses::Tokenize(key, substrings[1], " ");

	  vector<const Moses::Factor*> factorKey;
	  for (size_t i = 0; i < key.size(); ++i) {
		  factorKey.push_back(fc.AddFactor(key[i], false));
	  }

	  m_root.insert(factorKey, LMScores(prob, backoff));
  }

}

void LanguageModel::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
	  m_path = value;
  }
  else if (key == "factor") {
	  m_factorType = Moses::Scan<Moses::FactorType>(value);
  }
  else if (key == "order") {
	  m_order = Moses::Scan<size_t>(value);
  }
  else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
}

const Moses::FFState* LanguageModel::EmptyHypothesisState(const Manager &mgr, const Phrase &input) const
{
	return new Moses::PointerState(&m_root);
}

void
LanguageModel::EvaluateInIsolation(const System &system,
		  const PhraseBase &source, const TargetPhrase &targetPhrase,
        Scores &scores,
        Scores *estimatedFutureScores) const
{

}

Moses::FFState* LanguageModel::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const Moses::FFState &prevState,
  Scores &score) const
{

	return new Moses::PointerState(&m_root);
}
