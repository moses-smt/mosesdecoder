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
