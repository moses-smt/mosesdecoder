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

class System;
class PhraseBase;
class Phrase;
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

	  // may have more factors than actually need, but not guaranteed.
	  // For SCFG decoding, the source contains non-terminals, NOT the raw
	  // source from the input sentence
	  virtual void
	  EvaluateInIsolation(const System &system,
			  const PhraseBase &source, const TargetPhrase &targetPhrase,
	          Scores &scores,
	          Scores *estimatedScore) const = 0;

protected:
	size_t m_startInd;
	size_t m_numScores;
	std::string m_name;
	std::vector<std::vector<std::string> > m_args;
	bool m_tuneable;

	virtual void SetParameter(const std::string& key, const std::string& value);
	virtual void ReadParameters();
	void ParseLine(const std::string &line);
};

#endif /* FEATUREFUNCTION_H_ */
