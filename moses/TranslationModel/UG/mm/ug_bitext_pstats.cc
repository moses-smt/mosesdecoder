// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include <boost/thread/locks.hpp>
#include "ug_bitext_pstats.h"

namespace sapt
{

#if UG_BITEXT_TRACK_ACTIVE_THREADS
  ThreadSafeCounter pstats::active;
#endif

  pstats::
  pstats(bool const track_sids) : raw_cnt(0), sample_cnt(0), good(0), sum_pairs(0), in_progress(0), track_sids(track_sids)
  {
    for (int i = 0; i <= LRModel::NONE; ++i)
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
	// while (int(indoc.size()) <= docid) indoc.push_back(0);
	++indoc[docid];
      }
  }

  size_t
  pstats::
  add(uint64_t pid, float const w, float const b,
      std::vector<unsigned char> const& a,
      uint32_t const cnt2,
      uint32_t fwd_o,
      uint32_t bwd_o, int const docid, uint32_t const sid)
  {
    boost::lock_guard<boost::mutex> guard(this->lock);
    jstats& entry = this->trg[pid];
    size_t ret = entry.add(w, b, a, cnt2, fwd_o, bwd_o, docid, sid, track_sids);
    if (this->good < entry.rcnt())
      {
        UTIL_THROW(util::Exception, "more joint counts than good counts:"
                   << entry.rcnt() << "/" << this->good << "!");
      }
    return ret;
  }

  void 
  pstats::
  wait() const
  {
    boost::unique_lock<boost::mutex> lock(this->lock);
    while (this->in_progress)
      this->ready.wait(lock);
  }

} // end of namespace sapt

