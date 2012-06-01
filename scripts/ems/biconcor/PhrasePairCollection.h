#include "Vocabulary.h"
#include "SuffixArray.h"
#include "TargetCorpus.h"
#include "Alignment.h"
#include "PhrasePair.h"
#include "Mismatch.h"

#pragma once

class PhrasePairCollection
{
public:
  typedef unsigned int INDEX;

private:
  SuffixArray *m_suffixArray;
  TargetCorpus *m_targetCorpus;
  Alignment *m_alignment;
  vector< vector<PhrasePair*> > m_collection;
	vector< Mismatch* > m_mismatch, m_unaligned;
  int m_size;
  int m_max_lookup;
  int m_max_pp_target;
  int m_max_pp;

public:
  PhrasePairCollection ( SuffixArray *, TargetCorpus *, Alignment * );
  ~PhrasePairCollection ();

  bool GetCollection( const vector< string > sourceString );
  void Print();
  void PrintHTML();
};

// sorting helper
struct CompareBySize {
  bool operator()(const vector<PhrasePair*> a, const vector<PhrasePair*> b ) const {
    return a.size() > b.size();
  }
};
