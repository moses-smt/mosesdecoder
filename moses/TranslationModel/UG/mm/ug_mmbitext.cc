// #include "ug_mmbitext.h"
// #include <algorithm>

// namespace Moses
// {
//   using namespace ugdiss;
//   using namespace std;

//   mmbitext::
//   pstats::
//   pstats()
//     : raw_cnt(0), sample_cnt(0), good(0), sum_pairs(0), in_progress(0)
//   {}

//   void
//   mmbitext::
//   pstats::
//   register_worker()
//   {
//     this->lock.lock();
//     ++this->in_progress;
//     this->lock.unlock();
//   }
  
//   void
//   pstats::
//   release()
//   {
//     this->lock.lock();
//     if (this->in_progress-- == 1) // last one - >we're done
//       this->ready.notify_all();
//     this->lock.unlock();
//   }

//   void
//   mmbitext::
//   open(string const base, string const L1, string L2)
//   {
//     T1.open(base+L1+".mct");
//     T2.open(base+L2+".mct");
//     Tx.open(base+L1+"-"+L2+".mam");
//     V1.open(base+L1+".tdx"); V1.iniReverseIndex();
//     V2.open(base+L2+".tdx"); V2.iniReverseIndex();
//     I1.open(base+L1+".sfa",&T1);
//     I2.open(base+L2+".sfa",&T2);
//     // lexscorer.open(base+L1+"-"+L2+".lex");
//     assert(T1.size() == T2.size());
//   }


//   mmbitext::
//   mmbitext()
//     : ag(NULL)
//   {
    
//   }

//   bool
//   mmbitext::
//   find_trg_phr_bounds(size_t const sid, size_t const start, size_t const stop,
// 		      size_t & s1, size_t & s2, size_t & e1, size_t & e2,
// 		      vector<uchar>* core_alignment, bool const flip) const
//   {
//     // if (core_alignment) cout << "HAVE CORE ALIGNMENT" << endl;
//     // a word on the core_alignment:
//     // since fringe words ([s1,...,s2),[e1,..,e2) if s1 < s2, or e1 < e2, respectively)
//     // are be definition unaligned, we store only the core alignment in *core_alignment
//     // it is up to the calling function to shift alignment points over for start positions
//     // of extracted phrases that start with a fringe word
//     char const* p = Tx.sntStart(sid);
//     char const* x = Tx.sntEnd(sid);
//     bitvector forbidden((flip ? T1 : T2).sntLen(sid));
//     size_t src,trg;
//     size_t lft = forbidden.size();
//     size_t rgt = 0;
//     vector<vector<ushort> > aln(T1.sntLen(sid));
//     while (p < x)
//       {
// 	if (flip) { p = binread(p,trg); assert(p<x); p = binread(p,src); }
// 	else      { p = binread(p,src); assert(p<x); p = binread(p,trg); }
// 	if (src < start || src >= stop) 
// 	  forbidden.set(trg);
// 	else
// 	  {
// 	    lft = min(lft,trg);
// 	    rgt = max(rgt,trg);
// 	    if (core_alignment) 
// 	      {
// 		if (flip) aln[trg].push_back(src);
// 		else      aln[src].push_back(trg);
// 	      }
// 	  }
//       }
// #if 0
//     cout << setw(5) << mctr << " " << setw(3) << xctr << " ";
//     for (size_t i = 0; i < forbidden.size(); ++i)
//       {
// 	if (i == lft) cout << '(';
// 	cout << (forbidden[i] ? 'x' : '-');
// 	if (i == rgt) cout << ')';
//       }
//     cout << endl;
// #endif
    
//     for (size_t i = lft; i <= rgt; ++i)
//       if (forbidden[i]) 
// 	return false;
    
//     s2 = lft;   for (s1 = s2; s1 && !forbidden[s1-1]; --s1);
//     e1 = rgt+1; for (e2 = e1; e2 < forbidden.size() && !forbidden[e2]; ++e2);
    
//     if (lft > rgt) return false;
//     if (core_alignment) 
//       {
// 	core_alignment->clear();
// 	if (flip)
// 	  {
// 	    for (size_t i = lft; i <= rgt; ++i)
// 	      {
// 		sort(aln[i].begin(),aln[i].end());
// 		BOOST_FOREACH(ushort x, aln[i])
// 		  {
// 		    core_alignment->push_back(i-lft);
// 		    core_alignment->push_back(x-start);
// 		  }
// 	      }
// 	  }
// 	else
// 	  {
// 	    for (size_t i = start; i < stop; ++i)
// 	      {
// 		BOOST_FOREACH(ushort x, aln[i])
// 		  {
// 		    core_alignment->push_back(i-start);
// 		    core_alignment->push_back(x-lft);
// 		  }
// 	      }
// 	  }
//       }
//     return lft <= rgt;
//   }

//   void
//   mmbitext::
//   prep(iter const& phrase)
//   {
//     prep2(phrase);
//   }

//   sptr<mmbitext::pstats> 
//   mmbitext::
//   prep2(iter const& phrase)
//   {
//     if (!ag) 
//       {
// 	ag = new agenda(*this);
// 	ag->add_workers(20);
//       }
//     typedef boost::unordered_map<uint64_t,sptr<pstats> > pcache_t;
//     uint64_t pid = phrase.getPid();
//     pcache_t & cache(phrase.root == &this->I1 ? cache1 : cache2);
//     pcache_t::value_type entry(pid,sptr<pstats>());
//     pair<pcache_t::iterator,bool> foo = cache.emplace(entry);
//     if (foo.second) foo.first->second = ag->add_job(phrase, 1000);
//     return foo.first->second;
//   }

//   sptr<mmbitext::pstats>
//   mmbitext::
//   lookup(iter const& phrase)
//   {
//     sptr<pstats> ret = prep2(phrase);
//     boost::unique_lock<boost::mutex> lock(ret->lock);
//     while (ret->in_progress)
//       ret->ready.wait(lock);
//     return ret;
//   }

//   void
//   mmbitext::
//   agenda::
//   worker::
//   operator()()
//   {
//     uint64_t sid=0, offset=0, len=0; // of the source phrase
//     bool     fwd=false;              // source phrase is L1
//     sptr<mmbitext::pstats> stats;
//     size_t s1=0, s2=0, e1=0, e2=0;
//     for (; ag.get_task(sid,offset,len,fwd,stats); )
//       {
// 	if (!stats) break;
// 	vector<uchar> aln;
// 	if (!ag.bitext.find_trg_phr_bounds
// 	    (sid, offset, offset+len, s1, s2, e1, e2, fwd ? &aln : NULL, !fwd))
// 	  {
// 	    stats->release();
// 	    continue;
// 	  }

// 	stats->lock.lock(); 
// 	stats->good += 1; 
// 	stats->lock.unlock();

// 	for (size_t k = 0; k < aln.size(); k += 2) 
// 	  aln[k] += s2 - s1;
// 	Token const* o = (fwd ? ag.bitext.T2 : ag.bitext.T1).sntStart(sid);
// 	float sample_weight = 1./((s2-s1+1)*(e2-e1+1));
// 	for (size_t s = s1; s <= s2; ++s)
// 	  {
// 	    iter b(&(fwd ? ag.bitext.I2 : ag.bitext.I1));
// 	    for (size_t i = s; i < e1; ++i)
// 	      assert(b.extend(o[i].id()));
// 	    for (size_t i = e1; i <= e2; ++i)
// 	      {
// 		stats->add(b,sample_weight,aln);
// 		if (i < e2) assert(b.extend(o[i].id()));
// 	      }
// 	    if (fwd && s < s2) 
// 	      for (size_t k = 0; k < aln.size(); k += 2) 
// 		--aln[k];
// 	  }
// 	stats->release();
//       }
//   }
  
//   void
//   mmbitext::
//   pstats::
//   add(mmbitext::iter const& trg_phrase, float const w, vector<uchar> const& a)
//   {
//     this->lock.lock();
//     jstats& entry = this->trg[trg_phrase.getPid()];
//     this->lock.unlock();
//     entry.add(w,a);
//   }

//   mmbitext::
//   agenda::
//   agenda(mmbitext const& thebitext)
//     : shutdown(false), doomed(0), bitext(thebitext)
//   {
    
//   }

//   mmbitext::
//   agenda::
//   ~agenda()
//   {
//     this->lock.lock();
//     this->shutdown = true;
//     this->ready.notify_all();
//     this->lock.unlock();
//     for (size_t i = 0; i < workers.size(); ++i)
//       workers[i]->join();
//   }

//   mmbitext::
//   ~mmbitext()
//   {
//     if (ag) delete ag;
//   }
 
//   sptr<mmbitext::pstats>
//   mmbitext::
//   agenda::
//   add_job(mmbitext::iter const& phrase, size_t const max_samples)
//   {
//     static boost::posix_time::time_duration nodelay(0,0,0,0); 

//     job j;
//     j.stats.reset(new mmbitext::pstats());
//     j.stats->register_worker();
//     j.stats->raw_cnt = phrase.approxOccurrenceCount();
//     j.max_samples = max_samples;
//     j.next = phrase.lower_bound(-1);
//     j.stop = phrase.upper_bound(-1);
//     j.len  = phrase.size();
//     j.ctr  = 0;
//     j.fwd  = phrase.root == &bitext.I1;

//     boost::unique_lock<boost::mutex> lk(this->lock);
//     joblist.push_back(j);
//     if (joblist.size() == 1)
//       {
// 	for (size_t i = 0; i < workers.size(); ++i)
// 	  {
// 	    if (workers[i]->timed_join(nodelay))
// 	      {
// 		workers[i] = sptr<boost::thread>(new boost::thread(worker(*this)));
// 	      }
// 	  }
//       }
//     return j.stats;
//   }

//   bool
//   mmbitext::
//   agenda::
//   get_task(uint64_t & sid, uint64_t & offset, uint64_t & len, 
// 	   bool & fwd, sptr<mmbitext::pstats> & stats)
//   {
//     boost::unique_lock<boost::mutex> lock(this->lock);
//     if (this->doomed || this->shutdown) 
//       {
// 	if (this->doomed) --this->doomed;
// 	return false;
//       }
//     // while (joblist.empty())
//     //   {
//     // 	cerr << "no jobs" << endl;
//     // 	this->ready.wait(lock);
//     // 	if (this->doomed || this->shutdown) 
//     // 	  {
//     // 	    if (this->doomed) --this->doomed;
//     // 	    return false;
//     // 	  }
//     //   }
//     while (joblist.size())
//       {
// 	if (joblist.front().step(sid,offset))
// 	  {
// 	    job const& j = joblist.front();
// 	    len   = j.len;
// 	    fwd   = j.fwd;
// 	    stats = j.stats;
// 	    stats->register_worker();
// 	    return true;
// 	  }
// 	joblist.front().stats->release();
// 	joblist.pop_front();
//       }
//     stats.reset();
//     return true;
//   }

//   bool
//   mmbitext::
//   agenda::
//   job::
//   step(uint64_t & sid, uint64_t & offset)
//   {
//     while (next < stop && stats->good < max_samples)
//       {
// 	next = tightread(tightread(next,stop,sid),stop,offset);
// 	{
// 	  boost::lock_guard<boost::mutex> lock(stats->lock);
// 	  if (stats->raw_cnt == ctr) ++stats->raw_cnt;
// 	  size_t rnum = randInt(stats->raw_cnt - ctr++);
// 	  // cout << stats->raw_cnt << " " << ctr-1 << " " 
// 	  // << rnum << " " << max_samples - stats->good << endl;
// 	  if (rnum < max_samples - stats->good)
// 	    {
// 	      stats->sample_cnt++;
// 	      return true;
// 	    }
// 	}
//       }
//     return false;
//   }


//   void
//   mmbitext::
//   agenda::
//   add_workers(int n)
//   {
//     static boost::posix_time::time_duration nodelay(0,0,0,0); 
//     boost::lock_guard<boost::mutex> lock(this->lock);
//     // house keeping: remove all workers that have finished
//     for (size_t i = 0; i < workers.size(); )
//       {
// 	if (workers[i]->timed_join(nodelay))
//   	  {
//   	    if (i + 1 < workers.size())
//   	      workers[i].swap(workers.back());
//   	    workers.pop_back();
//   	  }
//   	else ++i;
//       }
//     if (n < 0) 
//       {
//   	this->doomed -= n;
//       }
//     else
//       {
//   	for (int i = 0; i < n; ++i)
// 	  {
// 	    sptr<boost::thread> w(new boost::thread(worker(*this)));
// 	    workers.push_back(w);
// 	  }
//       }
//   }

//   mmbitext::
//   jstats::
//   jstats()
//   { 
//     my_aln.reserve(1); 
//   }

//   mmbitext::
//   jstats::
//   jstats(jstats const& other)
//   {
//     my_rcnt = other.rcnt();
//     my_wcnt = other.wcnt();
//     my_aln  = other.aln();
//   }
  
//   void 
//   mmbitext::
//   jstats::
//   add(float w, vector<uchar> const& a)
//   {
//     boost::lock_guard<boost::mutex> lk(this->lock);
//     my_rcnt += 1;
//     my_wcnt += w;
//     if (a.size())
//       {
// 	size_t i = 0;
// 	while (i < my_aln.size() && my_aln[i].second != a) ++i;
// 	if (i == my_aln.size()) 
// 	  my_aln.push_back(pair<size_t,vector<uchar> >(1,a));
// 	else
// 	  my_aln[i].first++;
// 	if (my_aln[i].first > my_aln[i/2].first)
// 	  push_heap(my_aln.begin(),my_aln.begin()+i+1);
//       }
//   }

//   uint32_t
//   mmbitext::
//   jstats::
//   rcnt() const 
//   { return my_rcnt; }

//   float
//   mmbitext::
//   jstats::
//   wcnt() const
//   { return my_wcnt; }

//   vector<pair<size_t, vector<uchar> > > const&
//   mmbitext::
//   jstats::
//   aln() const 
//   { return my_aln; }

// }

