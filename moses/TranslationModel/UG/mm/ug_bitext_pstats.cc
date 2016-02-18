// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include <boost/thread/locks.hpp>
#include "ug_bitext_pstats.h"
#include <boost/format.hpp>

namespace sapt
{

#if UG_BITEXT_TRACK_ACTIVE_THREADS
  ThreadSafeCounter pstats::active;
#endif

  pstats::
  pstats() : raw_cnt(0), sample_cnt(0), good(0), sum_pairs(0), in_progress(0)
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
      uint32_t bwd_o, int const docid)
  {
    boost::lock_guard<boost::mutex> guard(this->lock);
    jstats& entry = this->trg[pid];
    size_t ret = entry.add(w, b, a, cnt2, fwd_o, bwd_o, docid);
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

  bool 
  pstats::
  operator==(pstats const& other) const
  {
    if (raw_cnt    != other.raw_cnt    || 
        sample_cnt != other.sample_cnt ||
        good       != other.good       || 
        sum_pairs  != other.sum_pairs  || 
        indoc      != other.indoc      ||
        trg        != other.trg) return false;

    for (int i =0; i <= LRModel::NONE; ++i)
      if (ofwd[i] != other.ofwd[i] || obwd[i] != other.obwd[i])
        return false;
      
    return true;
  }

  /// 
  void 
  pstats::
  diff(std::ostream& out, pstats const& other) const
  {
    boost::format fmt("%20s: %10d %10d\n");
    boost::format indoc_fmt("indoc %4d: %10d %10d\n");
    out << (fmt % "raw_cnt" % raw_cnt % other.raw_cnt);
    out << (fmt % "sample_cnt" % sample_cnt % other.sample_cnt);
    out << (fmt % "good" % good % other.good);
    out << (fmt % "sum_pairs" % sum_pairs % other.sum_pairs);
    indoc_map_t::const_iterator m1,m2;
    for (m1 = indoc.begin(); m1 != indoc.end(); ++m1)
      {
        m2 = other.indoc.find(m1->first);
        size_t cnt2 = m2 == other.indoc.end() ? 0 : m2->second;
        out << (indoc_fmt % m1->first % m1->second % cnt2);
      }
    for (m2 = other.indoc.begin(); m2 != other.indoc.end(); ++m2)
      {
        m1 = indoc.find(m2->first);
        size_t cnt1 = m1 == indoc.end() ? 0 : m1->second;
        out << (indoc_fmt % m2->first % cnt1 % m2->second);
      }

    for (int i =0; i <= LRModel::NONE; ++i)
      out << (fmt % "lrmdl fwd" % ofwd[i] % other.ofwd[i]); 

    for (int i =0; i <= LRModel::NONE; ++i)
      out << (fmt % "lrmdl bwd" % obwd[i] % other.obwd[i]); 
  }


} // end of namespace sapt

