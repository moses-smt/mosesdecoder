// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#include "phrasetable.h"
#include "statistic.h"
#include "timestamp.h"

#include <iostream>
#include <limits>
#include <sstream>
#include <string>

/* PhraseText */

PhraseText::DictionaryType_ PhraseText::dictionary_;
Count PhraseText::last_id_ = 1;

PhraseText::PhraseText(const String &s)
{
  IStringStream is(s);
  while(is.good()) {
    String w;
    getline(is, w, ' ');
    Count *id = boost::fast_pool_allocator<Count>::allocate(1);
    *id = index_word(w);
    word_list_.push_back(id);
  }
}

std::ostream &operator<<(std::ostream &os, const PhraseText &pt)
{
  bool print_space = false;
  for(PhraseText::const_string_iterator it = pt.string_begin(); it != pt.string_end(); it++) {
    if(print_space)
      os << ' ';
    else
      print_space = true;

    os << *it;
  }

  return os;
}

/* PhraseAlignment */

PhraseAlignment::Alignment::AlignmentMapType_ PhraseAlignment::Alignment::alignment_map_;
PhraseAlignment::Alignment::AlignmentVectorType_ PhraseAlignment::Alignment::alignment_vector_;

PhraseAlignment::Alignment::Alignment(Count slen, Count tlen, const String &alignment) :
  slen_(slen), tlen_(tlen), matrix_(slen * tlen, false)
{
  assert(slen_ > 0 && slen_ < 10);
  IStringStream is(alignment);
  while(is.good()) {
    String a;
    getline(is, a, ' ');
    IStringStream ap(a);
    Count s, t;
    char dash;
    ap >> s >> dash >> t;
    assert(s < slen && t < tlen);
    assert(dash == '-');
    matrix_[t * slen + s] = true;
  }
}

Count PhraseAlignment::Alignment::index_alignment(Count slen, Count tlen, const String &alignment)
{
  AlignmentTuple_ tup = boost::make_tuple(slen, tlen, alignment);
  AlignmentMapType_::const_iterator it = alignment_map_.find(tup);

  if(it == alignment_map_.end()) {
    const Alignment *pa = new Alignment(slen, tlen, alignment);
    Count index = alignment_vector_.size();
    alignment_map_.insert(std::make_pair(tup, index));
    alignment_vector_.push_back(pa);
    return index;
  } else
    return it->second;
}

std::ostream &operator<<(std::ostream &os, const PhraseAlignment::Alignment &pa)
{
  bool print_space = false;
  for(Count i = 0; i < pa.matrix_.size(); i++) {
    if(print_space)
      os << ' ';
    else
      print_space = true;

    os << (i / pa.slen_) << '-' << (i % pa.slen_);
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const PhraseAlignment &pa)
{
  for(Count s = 0; s < pa.get_source_length(); s++) {
    os << '(';
    bool print_comma = false;
    for(Count t = 0; t < pa.get_target_length(); t++) {
      if(pa.is_aligned(s, t)) {
        if(print_comma)
          os << ',';
        else
          print_comma = true;

        os << t;
      }
    }
    os << ") ";
  }

  os << "|||";

  for(Count t = 0; t < pa.get_target_length(); t++) {
    os << " (";
    bool print_comma = false;
    for(Count s = 0; s < pa.get_source_length(); s++) {
      if(pa.is_aligned(s, t)) {
        if(print_comma)
          os << ',';
        else
          print_comma = true;

        os << s;
      }
    }
    os << ')';
  }

  return os;
}

/* PhrasePairInfo */

bool PhrasePairInfo::init_phase_ = true;
Count PhrasePairInfo::data_ncounts_ = COUNT_FREE_IDX;
Count PhrasePairInfo::data_nscores_ = SCORE_FREE_IDX;
const Count PhrasePairInfo::CONTINUATION_BIT = 1 << (std::numeric_limits<Count>::digits - 1);

PhrasePairInfo::PhrasePairInfo(Count src, Count tgt, Count alignment, Count count) :
  src_(src), tgt_(tgt), data_(NULL), reverse_(false)
{
  init_phase_ = false;
  realloc_data(1);
  count_data(COUNT_COUNT_IDX) = count;
  Count *aligd = alignment_data(0);
  aligd[0] = alignment;
  aligd[1] = count;
}

DataIndex PhrasePairInfo::register_score_data(Count size)
{
  assert(init_phase_);

  Count start = data_nscores_;
  data_nscores_ += size;
  return start;
}

DataIndex PhrasePairInfo::register_count_data(Count size)
{
  assert(init_phase_);

  Count start = data_ncounts_;
  data_ncounts_ += size;
  return start;
}

PhrasePairInfo::AlignmentVector PhrasePairInfo::get_alignments() const
{
  PhrasePairInfo::AlignmentVector vec;

  Count i = 0;
  bool last;
  do {
    const Count *aligd = alignment_data(i++);
    last = !(aligd[0] & CONTINUATION_BIT);
    Count alig = aligd[0] & ~CONTINUATION_BIT;
    vec.push_back(std::make_pair(PhraseAlignment(alig, reverse_), aligd[1]));
  } while(!last);

  return vec;
}

void PhrasePairInfo::add_alignment(Count new_alignment)
{
  Count i = 0;
  bool last;
  do {
    Count *aligd = alignment_data(i++);
    last = !(aligd[0] & CONTINUATION_BIT);
    Count alig = aligd[0] & ~CONTINUATION_BIT;
    if(alig == new_alignment) {
      aligd[1]++;
      return;
    }
  } while(!last);

  realloc_data(i + 1);

  Count *last_aligd = alignment_data(i - 1);
  last_aligd[0] |= CONTINUATION_BIT;

  Count *this_aligd = alignment_data(i);
  this_aligd[0] = new_alignment;
  this_aligd[1] = 1;
}

void PhrasePairInfo::realloc_data(Count nalignments)
{
  static boost::pool<> *pool[3] = { NULL, NULL, NULL };

  size_t fixed_size = data_nscores_ * sizeof(Score) + data_ncounts_ * sizeof(Count);
  size_t new_data_size = fixed_size + COUNTS_PER_ALIGNMENT * nalignments * sizeof(Count);

  PhrasePairData new_data;
  if(nalignments <= 3) {
    if(!pool[nalignments - 1])
      pool[nalignments - 1] = new boost::pool<>(new_data_size);

    new_data = reinterpret_cast<PhrasePairData>(pool[nalignments - 1]->malloc());
  } else
    new_data = new char[new_data_size];

  if(data_) {
    memcpy(new_data, data_, fixed_size);
    Count i = 0;
    Count *old_aligd, *new_aligd;
    do {
      assert(i < nalignments);
      old_aligd = alignment_data(data_, i);
      new_aligd = alignment_data(new_data, i);
      new_aligd[0] = old_aligd[0];
      new_aligd[1] = old_aligd[1];
      i++;
    } while(old_aligd[0] & CONTINUATION_BIT);
    if(nalignments <= 4)
      pool[nalignments - 2]->free(data_);
    else
      delete[] data_;
  }

  data_ = new_data;
}

/* PhraseInfoList */

Phrase PhraseInfoList::index_phrase(const String &s_phr)
{
  IDMapType_::const_iterator it = idmap_.find(s_phr);
  if(it != idmap_.end())
    return it->second;

  PhraseInfo *pi = phrase_info_pool_.construct(data_size_, s_phr);

  list_.push_back(pi);
  idmap_[s_phr] = list_.size() - 1;
  return idmap_[s_phr];
}

DataIndex PhraseInfoList::register_data(Count size)
{
  DataIndex start = data_size_;
  data_size_ += size;
  return start;
}

void PhraseInfoList::attach_statistic(PhraseStatistic &s)
{
  statistics_.push_back(&s);
  s.attach(*this);
}

void PhraseInfoList::compute_statistics()
{
  while(!statistics_.empty()) {
    statistics_.front()->compute_statistic();
    statistics_.pop_front();
  }
}

/* PhraseTable */

void MemoryPhraseTable::load_data(std::istream &instream)
{
  Count total_count = 0;

  Timestamp t_load;
  Count nlines = 1;
  String line;
  while(getline(instream, line)) {
    size_t sep1 = line.find(" ||| ");
    if(sep1 == line.npos) {
      std::cerr << "Phrase separator not found in: " << line << std::endl;
      abort();
    }
    size_t sep2 = line.find(" ||| ", sep1 + 1);
    String s_src(line, 0, sep1);
    String s_tgt(line, sep1 + 5, sep2 - sep1 - 5);
    String s_alignment(line, sep2 + 5);

    Phrase src = src_info_.index_phrase(s_src);
    Phrase tgt = tgt_info_.index_phrase(s_tgt);
    Count alignment = PhraseAlignment::index_alignment(src_info_[src].get_phrase().size(), tgt_info_[tgt].get_phrase().size(), s_alignment);

    src_info_[src].inc_count();
    tgt_info_[tgt].inc_count();
    total_count++;

    PhrasePair stpair(src, tgt);
    PhrasePairCounts::iterator it = joint_counts_.find(stpair);

    if(it == joint_counts_.end()) {
      src_info_[src].inc_distinct();
      tgt_info_[tgt].inc_distinct();
      joint_counts_.insert(std::make_pair(stpair, PhrasePairInfo(src, tgt, alignment, 1).get_phrase_pair_data()));
    } else {
      PhrasePairInfo pi(src, tgt, it->second);
      pi.inc_count();
      pi.add_alignment(alignment);
      it->second = pi.get_phrase_pair_data(); // may have changed by adding the alignment
    }
    if(nlines % 50000 == 0)
      std:: cerr << "Read " << nlines << " lines in " << (t_load.elapsed_time() / 1000) << " ms." << std::endl;
    nlines++;
  }
}

void MemoryPhraseTable::attach_src_statistic(PhraseStatistic &s)
{
  src_info_.attach_statistic(s);
}

void MemoryPhraseTable::attach_tgt_statistic(PhraseStatistic &s)
{
  tgt_info_.attach_statistic(s);
}

void MemoryPhraseTable::compute_phrase_statistics()
{
  src_info_.compute_statistics();
  tgt_info_.compute_statistics();
}
