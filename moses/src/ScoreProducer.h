// $Id$

#ifndef moses_ScoreProducer_h
#define moses_ScoreProducer_h

#include <set>
#include <string>
#include <vector>

#include "FeatureVector.h"

namespace Moses
{

 /*
 * @note do not confuse this with a producer/consumer pattern.
 * this is not a producer in that sense.
 */
class ScoreProducer
{
private:
  std::string m_description;
  bool m_reportSparseFeatures;
  size_t m_numScoreComponents;
  //In case there's multiple producers with the same description
  static std::multiset<std::string> description_counts;
	ScoreProducer(const ScoreProducer&);  // don't implement
	
protected:
	ScoreProducer(const std::string& description, size_t numScoreComponents);
	virtual ~ScoreProducer();

public:

  static const size_t unlimited;

  static void ResetDescriptionCounts() {
    description_counts.clear();
  }

	//! returns the number of scores that a subclass produces.
	//! For example, a language model conventionally produces 1, a translation table some arbitrary number, etc
  //! sparse features returned unlimited
	size_t GetNumScoreComponents() const {return m_numScoreComponents;}

	//! returns a string description of this producer
	const std::string& GetScoreProducerDescription() const {return m_description;}

  //! returns the weight parameter name of this producer (used in n-best list)
  virtual std::string GetScoreProducerWeightShortName(unsigned idx=0) const = 0;

  //! returns the number of scores gathered from the input (0 by default)
  virtual size_t GetNumInputScores() const {
    return 0;
  };

	virtual bool IsStateless() const = 0;

  void SetSparseFeatureReporting() { m_reportSparseFeatures = true; }
  bool GetSparseFeatureReporting() const { return m_reportSparseFeatures; } 

  virtual float GetSparseProducerWeight() const { return 1; }
};


}

#endif
