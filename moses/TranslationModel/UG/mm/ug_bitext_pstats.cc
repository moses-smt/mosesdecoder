#include "ug_bitext_pstats.h"

namespace Moses
{
  namespace bitext
  {

#if UG_BITEXT_TRACK_ACTIVE_THREADS
    ThreadSafeCounter pstats::active;
#endif

    pstats::
    pstats() : raw_cnt(0), sample_cnt(0), good(0), sum_pairs(0), in_progress(0)
    {
      for (int i = 0; i <= Moses::LRModel::NONE; ++i)
	ofwd[i] = obwd[i] = 0;
    }

    pstats::
    ~pstats()
    {
#if UG_BITEXT_TRACK_ACTIVE_THREADS
      // counter may not exist any more at destruction time, so try ... catch
      try { --active; } catch (...) {}
#endif
    }

    void
    pstats::
    register_worker()
    {
      this->lock.lock();
      ++this->in_progress;
      this->lock.unlock();
    }

    void
    pstats::
    release()
    {
      this->lock.lock();
      if (this->in_progress-- == 1) // last one - >we're done
	this->ready.notify_all();
      this->lock.unlock();
    }

    void
    pstats
    ::count_sample(int const docid, size_t const num_pairs,
		   int const po_fwd, int const po_bwd)
    {
      boost::lock_guard<boost::mutex> guard(lock);
      ++sample_cnt;
      if (num_pairs == 0) return;
      ++good;
      sum_pairs += num_pairs;
      ++ofwd[po_fwd];
      ++obwd[po_bwd];
      if (docid >= 0)
	{
	  while (int(indoc.size()) <= docid) indoc.push_back(0);
	  ++indoc[docid];
	}
    }

    bool
    pstats::
    add(uint64_t pid, float const w,
	vector<uchar> const& a,
	uint32_t const cnt2,
	uint32_t fwd_o,
	uint32_t bwd_o, int const docid)
    {
      boost::lock_guard<boost::mutex> guard(this->lock);
      jstats& entry = this->trg[pid];
      entry.add(w, a, cnt2, fwd_o, bwd_o, docid);
      if (this->good < entry.rcnt())
	{
	  UTIL_THROW(util::Exception, "more joint counts than good counts:"
		     << entry.rcnt() << "/" << this->good << "!");
	}
      return true;
    }

  }
}
