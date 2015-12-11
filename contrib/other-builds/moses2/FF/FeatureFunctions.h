/*
 * FeatureFunctions.h
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#ifndef FEATUREFUNCTIONS_H_
#define FEATUREFUNCTIONS_H_

#include <vector>
#include <string>
#include "../legacy/Parameter.h"

class System;
class FeatureFunction;
class StatefulFeatureFunction;
class BatchedFeatureFunction;
class PhraseTable;
class Manager;
class MemPool;
class Phrase;
class PhraseImpl;
class TargetPhrase;
class Scores;

class FeatureFunctions {
public:
    std::vector<const FeatureFunction*> hasVocabInd;

    FeatureFunctions(System &system);
	virtual ~FeatureFunctions();

	const std::vector<const StatefulFeatureFunction*> &GetStatefulFeatureFunctions() const
	{ return m_statefulFeatureFunctions; }

	const std::vector<const BatchedFeatureFunction*> &GetBatchedFeatureFunctions() const
	{ return m_batchedFeatureFunctions; }

	size_t GetNumScores() const
	{ return m_ffStartInd; }

	void Create();
    void Load();

    const FeatureFunction &FindFeatureFunction(const std::string &name) const;
    const PhraseTable *GetPhraseTablesExcludeUnknownWordPenalty(size_t ptInd);

	  virtual void
	  EvaluateInIsolation(MemPool &pool, const System &system,
			  const Phrase &source, TargetPhrase &targetPhrase) const;

protected:
	  std::vector<const FeatureFunction*> m_featureFunctions;
	  std::vector<const StatefulFeatureFunction*> m_statefulFeatureFunctions;
		std::vector<const BatchedFeatureFunction*> m_batchedFeatureFunctions;
	  std::vector<const PhraseTable*> m_phraseTables;

	  System &m_system;
	  size_t m_ffStartInd;

	FeatureFunction *Create(const std::string &line);

};

#endif /* FEATUREFUNCTIONS_H_ */
