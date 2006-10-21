// $Id$

#ifndef _SCORE_PRODUCER_H_
#define _SCORE_PRODUCER_H_

#include <string>
#include <limits>

class ScoreIndexManager;

/** to keep track of the various things that can produce a score,
 * we use this evil implementation-inheritance to give them each
 * a unique, sequential (read: good for vector indices) ID
 *
 * @note do not confuse this with a producer/consumer pattern.
 * this is not a producer in that sense.
 */
class ScoreProducer
{
private:
	static unsigned int s_globalScoreBookkeepingIdCounter;
	unsigned int m_scoreBookkeepingId;

	ScoreProducer(const ScoreProducer&);  // don't implement
	
	 #define UNASSIGNED std::numeric_limits<unsigned int>::max()
	
protected:
	// it would be nice to force registration here, but some Producer objects
	// are constructed before they know how many scores they have
	ScoreProducer();
	virtual ~ScoreProducer();

public:
	//! contiguous id
	unsigned int GetScoreBookkeepingID() const { return m_scoreBookkeepingId; }
	void CreateScoreBookkeepingID()	{	m_scoreBookkeepingId = s_globalScoreBookkeepingIdCounter++;}
	//! returns the number of scores that a subclass produces.
	//! For example, a language model conventionally produces 1, a translation table some arbitrary number, etc
	virtual size_t GetNumScoreComponents() const = 0;

	//! returns a string description of this producer
	virtual const std::string GetScoreProducerDescription() const = 0;
};

#endif
