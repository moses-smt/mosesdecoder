/*
 * PhraseDictionaryCache.h
 *
 *  Created on: Dec 5, 2012
 *      Author: prashant
 */

#ifndef PhraseDictionaryCache_H_
#define PhraseDictionaryCache_H_

#include "moses/FeatureFunction.h"
#include "moses/InputFileStream.h"
#include "moses/TargetPhraseCollection.h"
#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#endif

#define CBTM_SCORE_TYPE_HYPERBOLA 0
#define CBTM_SCORE_TYPE_POWER 1
#define CBTM_SCORE_TYPE_EXPONENTIAL 2
#define CBTM_SCORE_TYPE_COSINE 3
#define CBTM_SCORE_TYPE_HYPERBOLA_REWARD 10
#define CBTM_SCORE_TYPE_POWER_REWARD 11
#define CBTM_SCORE_TYPE_EXPONENTIAL_REWARD 12
#define PI 3.14159265

namespace Moses {
typedef std::pair<int, size_t> AgePosPair;
typedef std::pair<Phrase, AgePosPair> TargetAgePosPair;
typedef std::map<Phrase, AgePosPair> TargetAgeMap;
typedef std::pair<TargetPhraseCollection*, TargetAgeMap*> TargetCollectionAgePair;


class PhraseDictionaryCache : public PhraseDictionary
{
	std::map<Phrase, TargetCollectionAgePair> m_cacheTM;
	std::vector<Scores> precomputedScores;
	unsigned int maxAge;
        size_t score_type; //scoring type of the match
#ifdef WITH_THREADS
  //multiple readers - single writer lock
  mutable boost::shared_mutex m_cacheLock;
#endif

protected:
	float decaying_score(int age);	// calculates the decay score given the age

	void Decay();	// traverse through the cache and decay each entry
	void Decay(Phrase p);	// traverse through the cache and decay each entry for a given Phrase
        void Update(std::string sourceString, std::string targetString, std::string ageString);
	void Update(Phrase p, Phrase tp, int age);
	void Execute(std::string command);
	void Clear();		// clears the cache
	void SetPreComputedScores(int numScoreComponent);
	void LoadCacheFile(const std::string &filePath);

        void SetScoreType(size_t type);
        void SetMaxAge(unsigned int age);

public:
	PhraseDictionaryCache(size_t numScoreComponentonst, std::string filePath, PhraseDictionaryFeature* feature, const size_t s_type, unsigned int age);
	virtual ~PhraseDictionaryCache();
	std::string GetScoreProducerDescription(unsigned) const;
	std::string GetScoreProducerWeightShortName(unsigned) const;
	size_t GetNumScoreComponents() const;

	void Print() const;		// prints the cache


	bool Load(const std::vector<FactorType> &input
						, const std::vector<FactorType> &output);
	
	void Insert(std::vector<std::string> entries);
	
	void Execute(std::vector<std::string> commands);
	
	const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase &source) const;

	virtual void InitializeForInput(InputType const&) {
		/* Don't do anything source specific here as this object is shared between threads.*/
	}

	virtual ChartRuleLookupManager *CreateRuleLookupManager(
			const InputType &,
			const ChartCellCollectionBase &) {
		CHECK(false);
		return 0;
	}

};

}

#endif /* PhraseDictionaryCache_H_ */
