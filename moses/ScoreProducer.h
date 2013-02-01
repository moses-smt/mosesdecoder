// $Id$

#ifndef moses_ScoreProducer_h
#define moses_ScoreProducer_h

#include <set>
#include <string>
#include <vector>

#include "FeatureVector.h"

namespace Moses
{
class InputType;

 /*
 * @note do not confuse this with a producer/consumer pattern.
 * this is not a producer in that sense.
 */
class ScoreProducer
{
protected:
  std::string m_description, m_argLine;
  std::vector<std::vector<std::string> > m_args;
  bool m_reportSparseFeatures;
  size_t m_numScoreComponents;
  //In case there's multiple producers with the same description
  static std::multiset<std::string> description_counts;

  ScoreProducer(const ScoreProducer&);  // don't implement
	ScoreProducer(const std::string& description, size_t numScoreComponents, const std::string &line);

	void ParseLine(const std::string &line);
  size_t FindNumFeatures();

public:

  static const size_t unlimited;

  static void ResetDescriptionCounts() {
    description_counts.clear();
  }

  virtual ~ScoreProducer();

	//! returns the number of scores that a subclass produces.
	//! For example, a language model conventionally produces 1, a translation table some arbitrary number, etc
  //! sparse features returned unlimited
	size_t GetNumScoreComponents() const {return m_numScoreComponents;}

	//! returns a string description of this producer
	const std::string& GetScoreProducerDescription() const
	{ return m_description; }

	virtual bool IsStateless() const = 0;

  void SetSparseFeatureReporting() { m_reportSparseFeatures = true; }
  bool GetSparseFeatureReporting() const { return m_reportSparseFeatures; } 

  virtual float GetSparseProducerWeight() const { return 1; }

  virtual bool IsTuneable() const { return true; }

  //!
  virtual void InitializeForInput(InputType const& source)
  {}

  // clean up temporary memory, called after processing each sentence
  virtual void CleanUpAfterSentenceProcessing(const InputType& source)
  {}

  const std::string &GetArgLine() const
  { return m_argLine; }
};


}

#endif
