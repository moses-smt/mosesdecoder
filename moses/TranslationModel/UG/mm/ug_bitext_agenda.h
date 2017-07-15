// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// to be included from ug_bitext.h

// The agenda handles parallel sampling.
// It maintains a queue of unfinished sampling jobs and
// assigns them to a pool of workers.
//
template<typename Token>
class Bitext<Token>
::agenda
{
public:
  class job;
  class worker;
private:
  boost::mutex lock;
  std::list<SPTR<job> > joblist;
  std::vector<SPTR<boost::thread> > workers;
  bool shutdown;
  size_t doomed;

public:


  Bitext<Token>   const& bt;

  agenda(Bitext<Token> const& bitext);
  ~agenda();

  void
  add_workers(int n);

  SPTR<pstats>
  add_job(Bitext<Token> const* const theBitext,
	  typename TSA<Token>::tree_iterator const& phrase,
	  size_t const max_samples, SPTR<SamplingBias const> const& bias,
    bool const track_sids);
    // add_job(Bitext<Token> const* const theBitext,
    // 	  typename TSA<Token>::tree_iterator const& phrase,
    // 	  size_t const max_samples, SamplingBias const* const bias);

  SPTR<job>
  get_job();
};

template<typename Token>
class
Bitext<Token>::agenda::
worker
{
  agenda& ag;
public:
  worker(agenda& a) : ag(a) {}
  void operator()();
};

#include "ug_bitext_agenda_worker.h"
#include "ug_bitext_agenda_job.h"

template<typename Token>
void Bitext<Token>
::agenda
::add_workers(int n)
{
  static boost::posix_time::time_duration nodelay(0,0,0,0);
  boost::lock_guard<boost::mutex> guard(this->lock);

  int target  = std::max(1, int(n + workers.size() - this->doomed));
  // house keeping: remove all workers that have finished
  for (size_t i = 0; i < workers.size(); )
    {
      if (workers[i]->timed_join(nodelay))
        {
          if (i + 1 < workers.size())
            workers[i].swap(workers.back());
          workers.pop_back();
        }
      else ++i;
    }
  // cerr << workers.size() << "/" << target << " active" << std::endl;
  if (int(workers.size()) > target)
    this->doomed = workers.size() - target;
  else
    while (int(workers.size()) < target)
      {
        SPTR<boost::thread> w(new boost::thread(worker(*this)));
        workers.push_back(w);
      }
}


template<typename Token>
SPTR<pstats> Bitext<Token>
::agenda
::add_job(Bitext<Token> const* const theBitext,
	  typename TSA<Token>::tree_iterator const& phrase,
	  size_t const max_samples, SPTR<SamplingBias const> const& bias,
	  bool const track_sids)
{
  boost::unique_lock<boost::mutex> lk(this->lock);
  static boost::posix_time::time_duration nodelay(0,0,0,0);
  bool fwd = phrase.root == bt.I1.get();
  SPTR<job> j(new job(theBitext, phrase, fwd ? bt.I1 : bt.I2,
		      max_samples, fwd, bias, track_sids));
  j->stats->register_worker();

  joblist.push_back(j);
  if (joblist.size() == 1)
    {
      size_t i = 0;
      while (i < workers.size())
	{
	  if (workers[i]->timed_join(nodelay))
	    {
	      if (doomed)
		{
		  if (i+1 < workers.size())
		    workers[i].swap(workers.back());
		  workers.pop_back();
		  --doomed;
		}
	      else
		workers[i++] = SPTR<boost::thread>(new boost::thread(worker(*this)));
	    }
	  else ++i;
	}
    }
  return j->stats;
}

template<typename Token>
SPTR<typename Bitext<Token>::agenda::job>
Bitext<Token>
::agenda
::get_job()
{
  // cerr << workers.size() << " workers on record" << std::endl;
  SPTR<job> ret;
  if (this->shutdown) return ret;
  boost::unique_lock<boost::mutex> lock(this->lock);
  if (this->doomed)
    { // the number of workers has been reduced, tell the redundant once to quit
      --this->doomed;
      return ret;
    }

  typename std::list<SPTR<job> >::iterator j = joblist.begin();
  while (j != joblist.end())
    {
      if ((*j)->done())
	{
	  (*j)->stats->release();
	  joblist.erase(j++);
	}
      else if ((*j)->workers >= 4) ++j; // no more than 4 workers per job
      else break; // found one
    }
  if (joblist.size())
    {
      ret = j == joblist.end() ? joblist.front() : *j;
      // if we've reached the end of the queue (all jobs have 4 workers on them),
      // take the first in the queue
      boost::lock_guard<boost::mutex> jguard(ret->lock);
      ++ret->workers;
    }
  return ret;
}

template<typename Token>
Bitext<Token>::
agenda::
~agenda()
{
  this->lock.lock();
  this->shutdown = true;
  this->lock.unlock();
  for (size_t i = 0; i < workers.size(); ++i)
    workers[i]->join();
}

template<typename Token>
Bitext<Token>::
agenda::
agenda(Bitext<Token> const& thebitext)
  : shutdown(false), doomed(0), bt(thebitext)
{ }


