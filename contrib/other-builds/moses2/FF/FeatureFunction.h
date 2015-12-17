/*
 * FeatureFunction.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#ifndef FEATUREFUNCTION_H_
#define FEATUREFUNCTION_H_

#include <cstddef>
#include <string>
#include <vector>

namespace Moses2
{

class System;
class Phrase;
class PhraseImpl;
class TargetPhrase;
class Scores;
class Manager;

class FeatureFunction {
public:

	FeatureFunction(size_t startInd, const std::string &line);
	virtual ~FeatureFunction();
	virtual void Load(System &system)
	{}

	size_t GetStartInd() const
	{ return m_startInd; }
	size_t GetNumScores() const
	{ return m_numScores; }
	const std::string &GetName() const
	{ return m_name; }

	virtual size_t HasPhraseTableInd() const
	{ return false; }
	void SetPhraseTableInd(size_t ind)
	{ m_PhraseTableInd = ind; }

	  // may have more factors than actually need, but not guaranteed.
	  // For SCFG decoding, the source contains non-terminals, NOT the raw
	  // source from the input sentence
	  virtual void
	  EvaluateInIsolation(const System &system,
			  const Phrase &source, const TargetPhrase &targetPhrase,
	          Scores &scores,
	          Scores *estimatedScores) const = 0;

	  // clean up temporary memory, called after processing each sentence
	  virtual void CleanUpAfterSentenceProcessing() {}

protected:
	size_t m_startInd;
	size_t m_numScores;
	size_t m_PhraseTableInd;
	std::string m_name;
	std::vector<std::vector<std::string> > m_args;
	bool m_tuneable;

	virtual void SetParameter(const std::string& key, const std::string& value);
	virtual void ReadParameters();
	void ParseLine(const std::string &line);
};

}


#endif /* FEATUREFUNCTION_H_ */
