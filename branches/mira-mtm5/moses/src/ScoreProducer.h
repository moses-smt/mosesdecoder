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
  mutable  std::vector<FName> m_names; //for features with fixed number of values
  std::string m_description;
  //In case there's multiple producers with the same description
  static std::multiset<std::string> description_counts;
	ScoreProducer(const ScoreProducer&);  // don't implement
	
protected:
	ScoreProducer(const std::string& description);
	virtual ~ScoreProducer();

public:
	//! returns the number of scores that a subclass produces.
	//! For example, a language model conventionally produces 1, a translation table some arbitrary number, etc
  //! will cause an error if this producer does not have a fixed number
  //! of scores (eg sparse features)
	virtual size_t GetNumScoreComponents() const = 0;

	//! returns a string description of this producer
	const std::string& GetScoreProducerDescription() const {return m_description;}

	//! returns the weight parameter name of this producer (used in n-best list)
	virtual std::string GetScoreProducerWeightShortName() const = 0;

	//! returns the number of scores gathered from the input (0 by default)
	virtual size_t GetNumInputScores() const { return 0; };

  const std::vector<FName>& GetFeatureNames() const;

	virtual bool IsStateless() const = 0;

};


}

#endif
