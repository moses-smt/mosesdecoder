// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// to be included from ug_bitext_agenda.h

template<typename Token>
void
Bitext<Token>::agenda
::worker
::operator()()
{
  // things to do:
  //
  // - have each worker maintain their own pstats object and merge
  //   results at the end (to minimize mutex locking);
  //
  // - use a non-locked, monotonically increasing counter to
  //   ensure the minimum size of samples considered --- it's OK if
  //   we look at more samples than required. This way, we can
  //   reduce the number of lock / unlock operations we need to do
  //   during sampling.

  uint64_t sid=0, offset=0;       // sid and offset of source phrase
  size_t s1=0, s2=0, e1=0, e2=0;  // soft and hard boundaries of target phrase
  std::vector<unsigned char> aln; // stores phrase-pair-internal alignment
  while(SPTR<job> j = ag.get_job())
    {
      j->stats->register_worker();
      bitvector full_alignment(100*100); // Is full_alignment still needed???
      while (j->nextSample(sid,offset))
	{
	  aln.clear();
	  int po_fwd = LRModel::NONE;
	  int po_bwd = LRModel::NONE;
	  int docid  = j->m_bias ? j->m_bias->GetClass(sid) : -1;
	  bitvector* full_aln = j->fwd ? &full_alignment : NULL;

	  // find soft and hard boundaries of target phrase
	  bool good = (ag.bt.find_trg_phr_bounds
		       (sid, offset, offset + j->len,   // input parameters
			s1, s2, e1, e2, po_fwd, po_bwd, // bounds & orientation
			&aln, full_aln, !j->fwd));      // aln info / flip sides?

	  if (!good)
	    { // no good, probably because phrase is not coherent
	      j->stats->count_sample(docid, 0, po_fwd, po_bwd);
	      continue;
	    }

	  // all good: register this sample as valid
	  size_t num_pairs = (s2-s1+1) * (e2-e1+1);
	  j->stats->count_sample(docid, num_pairs, po_fwd, po_bwd);

#if 0
	  Token const* t = ag.bt.T2->sntStart(sid);
	  Token const* eos = ag.bt.T2->sntEnd(sid);
	  cerr << "[" << j->stats->good + 1 << "] ";
	  while (t != eos) cerr << (*ag.bt.V2)[(t++)->id()] << " ";
	  cerr << "[" << docid << "]" << std::endl;
#endif

	  float sample_weight = 1./num_pairs;
	  Token const* o = (j->fwd ? ag.bt.T2 : ag.bt.T1)->sntStart(sid);

	  // adjust offsets in phrase-internal aligment
	  for (size_t k = 1; k < aln.size(); k += 2) aln[k] += s2 - s1;

	  std::vector<uint64_t> seen; seen.reserve(10);
	  // It is possible that the phrase extraction extracts the same
	  // phrase twice, e.g., when word a co-occurs with sequence b b b
	  // but is aligned only to the middle word. We can only count
	  // each phrase std::pair once per source phrase occurrence, or else
	  // run the risk of having more joint counts than marginal
	  // counts.

	  for (size_t s = s1; s <= s2; ++s)
	    {
	      TSA<Token> const& I = j->fwd ? *ag.bt.I2 : *ag.bt.I1;
	      SPTR<iter> b = I.find(o + s, e1 - s);
	      UTIL_THROW_IF2(!b || b->size() < e1-s, "target phrase not found");

	      for (size_t i = e1; i <= e2; ++i)
		{
		  uint64_t tpid = b->getPid();

		  // poor man's protection against over-counting
		  size_t s = 0;
		  while (s < seen.size() && seen[s] != tpid) ++s;
		  if (s < seen.size()) continue;
		  seen.push_back(tpid);

		  size_t raw2 = b->approxOccurrenceCount();
		  float bwgt = j->m_bias ? (*j->m_bias)[sid] : 1;
		  j->stats->add(tpid, sample_weight, bwgt, aln, raw2,
				po_fwd, po_bwd, docid, sid);
		  bool ok = (i == e2) || b->extend(o[i].id());
		  UTIL_THROW_IF2(!ok, "Could not extend target phrase.");
		}
	      if (s < s2) // shift phrase-internal alignments
		for (size_t k = 1; k < aln.size(); k += 2)
		  --aln[k];
	    }
	}
      j->stats->release(); // indicate that you're done working on j->stats
    }
}
