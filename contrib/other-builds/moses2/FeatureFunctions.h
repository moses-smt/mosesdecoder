/*
 * FeatureFunctions.h
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#ifndef FEATUREFUNCTIONS_H_
#define FEATUREFUNCTIONS_H_

#include <vector>
#include "moses/Parameter.h"

class System;
class FeatureFunction;
class StatefulFeatureFunction;
class PhraseTable;

class FeatureFunctions {
public:
	FeatureFunctions(System &system);
	virtual ~FeatureFunctions();

	const std::vector<const PhraseTable*> &GetPhraseTables() const
	{ return m_phraseTables; }

	const std::vector<const StatefulFeatureFunction*> &GetStatefulFeatureFunctions() const
	{ return m_statefulFeatureFunctions; }

	size_t GetFFStartInd() const
	{ return m_ffStartInd; }

    void LoadFeatureFunctions();

protected:
	  std::vector<const FeatureFunction*> m_featureFunctions;
	  std::vector<const StatefulFeatureFunction*> m_statefulFeatureFunctions;
	  std::vector<const PhraseTable*> m_phraseTables;

	  System &m_system;
	  size_t m_ffStartInd;

	FeatureFunction *Create(const std::string &line);

};

#endif /* FEATUREFUNCTIONS_H_ */
