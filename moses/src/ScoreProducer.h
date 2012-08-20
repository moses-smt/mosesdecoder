// $Id$

#ifndef moses_ScoreProducer_h
#define moses_ScoreProducer_h

#include <string>
#include <limits>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

namespace Moses
{

class Hypothesis;
class ScoreComponentCollection;
class ScoreIndexManager;
class FFState;

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
#ifdef WITH_THREADS
  boost::mutex m_mutex;
  static std::map<boost::thread::id, unsigned int> s_globalScoreBookkeepingIdCounterThreadMap;  
#else
  static unsigned int s_globalScoreBookkeepingIdCounter;
#endif
  unsigned int m_scoreBookkeepingId;

#define UNASSIGNED std::numeric_limits<unsigned int>::max()

protected:
  // it would be nice to force registration here, but some Producer objects
  // are constructed before they know how many scores they have
  ScoreProducer();
  virtual ~ScoreProducer();
  
  //MJD: Implemented to allow shallow and empty copies
  ScoreProducer(const ScoreProducer& sp) {}

public:
  
  //! contiguous id
  unsigned int GetScoreBookkeepingID() const {
    return m_scoreBookkeepingId;
  }
  void CreateScoreBookkeepingID() {
#ifdef WITH_THREADS
    boost::mutex::scoped_lock lock(m_mutex);
    unsigned int& counterRef = s_globalScoreBookkeepingIdCounterThreadMap[boost::this_thread::get_id()];
#else
    unsigned int& counterRef = s_globalScoreBookkeepingIdCounter;
#endif
    m_scoreBookkeepingId = counterRef++;
  }
  //! returns the number of scores that a subclass produces.
  //! For example, a language model conventionally produces 1, a translation table some arbitrary number, etc
  virtual size_t GetNumScoreComponents() const = 0;

  //! returns a string description of this producer
  virtual std::string GetScoreProducerDescription(unsigned idx=0) const = 0;

  //! returns the weight parameter name of this producer (used in n-best list)
  virtual std::string GetScoreProducerWeightShortName(unsigned idx=0) const = 0;

  //! returns the number of scores gathered from the input (0 by default)
  virtual size_t GetNumInputScores() const {
    return 0;
  };

  virtual bool IsStateless() const = 0;

};


}

#endif
