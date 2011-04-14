// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#ifndef PHRASETABLE_H
#define PHRASETABLE_H

#include <cassert>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/bimap.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include "datastorage.h"
#include "memscore.h"

class PhraseText
{
  friend std::ostream &operator<<(std::ostream &os, const PhraseText &pt);

private:
  typedef boost::ptr_vector<Count,boost::view_clone_allocator> WordListType_;
  typedef boost::bimap<String,Count> DictionaryType_;

  WordListType_ word_list_;

  static DictionaryType_ dictionary_;
  static Count last_id_;

  typedef const String &(*LookupFunction_)(Count id);

public:
  typedef WordListType_::const_iterator const_iterator;
  typedef boost::transform_iterator<LookupFunction_,WordListType_::const_iterator> const_string_iterator;
  typedef WordListType_::size_type size_type;

  PhraseText(const String &s);

  const_iterator begin() const {
    return word_list_.begin();
  }

  const_iterator end() const {
    return word_list_.end();
  }

  const_string_iterator string_begin() const {
    return boost::make_transform_iterator(word_list_.begin(), dictionary_lookup);
  }

  const_string_iterator string_end() const {
    return boost::make_transform_iterator(word_list_.end(), dictionary_lookup);
  }

  Count operator[](size_type i) const {
    return word_list_[i];
  }

  const String &word(size_type i) const {
    return dictionary_lookup(operator[](i));
  }

  size_type size() const {
    return word_list_.size();
  }

  static const String &dictionary_lookup(Count id) {
    DictionaryType_::right_const_iterator it = dictionary_.right.find(id);
    assert(it != dictionary_.right.end());
    return it->second;
  }

  static Count index_word(const String &word) {
    Count id;
    DictionaryType_::left_const_iterator it = dictionary_.left.find(word);
    if(it != dictionary_.left.end())
      id = it->second;
    else {
      id = last_id_++;
      dictionary_.insert(DictionaryType_::value_type(word, id));
    }
    return id;
  }
};

class PhraseInfo
{
  friend class boost::object_pool<PhraseInfo>;
  friend std::ostream &operator<<(std::ostream &os, const PhraseInfo &pt);

protected:
  Count data_size_;

  Count count_;
  Count distinct_;
  PhraseText phrase_;
  Score *data_;

  Count n1_;
  Count n2_;
  Count n3plus_;

  PhraseInfo(Count data_size, const String &phrase) :
    data_size_(data_size), count_(0), distinct_(0), phrase_(phrase), n1_(0), n2_(0), n3plus_(0) {
    data_ = DataStorage<Score>::get_instance().alloc(data_size_);
  }

public:
  Score &data(Count base, Count i = 0) {
    assert(base + i < data_size_);
    return *(data_ + base + i);
  }

  const Score &data(Count base, Count i = 0) const {
    assert(base + i < data_size_);
    return *(data_ + base + i);
  }

  Count get_count() const {
    return count_;
  }

  void inc_count() {
    count_++;
  }

  Count get_distinct() const {
    return distinct_;
  }

  void inc_distinct() {
    distinct_++;
  }

  const PhraseText &get_phrase() const {
    return phrase_;
  }

  void inc_n1() {
    n1_++;
  }

  Count get_n1() {
    return n1_;
  }

  void inc_n2() {
    n2_++;
  }

  Count get_n2() {
    return n2_;
  }

  void inc_n3plus() {
    n3plus_++;
  }

  Count get_n3plus() {
    return n3plus_;
  }

};

inline std::ostream &operator<<(std::ostream &os, const PhraseInfo &pt)
{
  return os << pt.get_phrase();
}

class PhraseInfoList
{
protected:
  typedef std::map<String,Phrase> IDMapType_;
  typedef boost::ptr_vector<PhraseInfo,boost::view_clone_allocator> ListType_;
  //typedef std::vector<PhraseInfo *,boost::pool_allocator<PhraseInfo *> > ListType_;
  //typedef std::vector<PhraseInfo> ListType_;
  typedef std::list<PhraseStatistic *> StatListType_;

  IDMapType_ idmap_;
  ListType_ list_;
  StatListType_ statistics_;
  boost::object_pool<PhraseInfo> phrase_info_pool_;

  Count data_size_;
public:
  typedef ListType_::iterator iterator;
  typedef ListType_::const_iterator const_iterator;
  typedef ListType_::size_type size_type;

  PhraseInfoList() : data_size_(0) {}

  Phrase index_phrase(const String &s_phr);
  DataIndex register_data(Count size);
  void attach_statistic(PhraseStatistic &s);
  void compute_statistics();

  PhraseInfo &operator[](Phrase phr) {
    return list_[phr];
  }

  iterator begin() {
    return list_.begin();
  }

  iterator end() {
    return list_.end();
  }

  const_iterator begin() const {
    return list_.begin();
  }

  const_iterator end() const {
    return list_.end();
  }

  size_type size() const {
    return list_.size();
  }

};

class PhraseAlignment
{
  friend std::ostream &operator<<(std::ostream &os, const PhraseAlignment &pa);

private:
  class Alignment
  {
    friend std::ostream &operator<<(std::ostream &os, const Alignment &pa);

  private:
    typedef boost::tuple<Count,Count,String> AlignmentTuple_;
    typedef std::map<AlignmentTuple_,Count> AlignmentMapType_;
    typedef std::vector<const Alignment *> AlignmentVectorType_;

    static AlignmentMapType_ alignment_map_;
    static AlignmentVectorType_ alignment_vector_;

    Count slen_, tlen_;
    boost::dynamic_bitset<unsigned int> matrix_;

    Alignment(Count slen, Count tlen, const String &alignment);

  public:
    bool is_aligned(Count s, Count t) const {
      assert(t < tlen_);
      assert(s < slen_);
      return matrix_[t * slen_ + s];
    }

    Count get_source_length() const {
      return slen_;
    }

    Count get_target_length() const {
      return tlen_;
    }

    bool operator<(const Alignment &pa) const {
      if(slen_ < pa.slen_) return true;
      if(tlen_ < pa.tlen_) return true;
      return (matrix_ < pa.matrix_);
    }

    static Count index_alignment(Count slen, Count tlen, const String &alignment);

    static const Alignment *find(Count index) {
      return alignment_vector_[index];
    }
  };
  friend std::ostream &operator<<(std::ostream &os, const Alignment &pa);

  const Alignment *alignment_;
  bool reverse_;

public:
  PhraseAlignment(Count index, bool reverse = false) :
    alignment_(Alignment::find(index)), reverse_(reverse) {}

  bool is_aligned(Count s, Count t) const {
    if(!reverse_)
      return alignment_->is_aligned(s, t);
    else
      return alignment_->is_aligned(t, s);
  }

  Count get_source_length() const {
    if(!reverse_)
      return alignment_->get_source_length();
    else
      return alignment_->get_target_length();
  }

  Count get_target_length() const {
    if(!reverse_)
      return alignment_->get_target_length();
    else
      return alignment_->get_source_length();
  }

  static Count index_alignment(Count slen, Count tlen, const String &alignment) {
    return Alignment::index_alignment(slen, tlen, alignment);
  }
};

typedef std::map<PhrasePair,PhrasePairData> PhrasePairCounts;

class PhrasePairInfo
{
protected:
  static const Count CONTINUATION_BIT;

  static bool init_phase_;
  static Count data_nscores_;
  static Count data_ncounts_;

  enum { COUNT_COUNT_IDX = 0, COUNT_FREE_IDX }; // COUNT_FREE_IDX must remain last!
  enum { SCORE_FREE_IDX = 0 }; // SCORE_FREE_IDX must remain last!

  Phrase src_, tgt_;
  PhrasePairData data_;
  bool reverse_;

  void realloc_data(Count nalignments);

public:
  typedef std::vector<std::pair<PhraseAlignment,Count> > AlignmentVector;

  static DataIndex register_score_data(Count size);
  static DataIndex register_count_data(Count size);

  PhrasePairInfo(Count src, Count tgt, Count alignment, Count count);

  PhrasePairInfo(Count src, Count tgt, PhrasePairData data, bool reverse = false) : src_(src), tgt_(tgt), data_(data), reverse_(reverse) {
    init_phase_ = false;
  }

  PhrasePairInfo(const PhrasePairCounts::const_iterator &in) :
    src_(in->first.first), tgt_(in->first.second), data_(in->second), reverse_(false) {}

  PhrasePairData get_phrase_pair_data() {
    return data_;
  }

  Phrase get_src() const {
    return !reverse_ ? src_ : tgt_;
  }

  Phrase get_tgt() const {
    return !reverse_ ? tgt_ : src_;
  }

  Count get_count() const {
    return count_data(COUNT_COUNT_IDX);
  }

  Score &score_data(DataIndex base, DataIndex index = 0) {
    return score_data(data_, base, index);
  }

  const Score &score_data(DataIndex base, DataIndex index = 0) const {
    return score_data(data_, base, index);
  }

  Count &count_data(DataIndex base, DataIndex index = 0) {
    return count_data(data_, base, index);
  }

  const Count &count_data(DataIndex base, DataIndex index = 0) const {
    return count_data(data_, base, index);
  }

  void inc_count() {
    count_data(data_, COUNT_COUNT_IDX)++;
  }

  AlignmentVector get_alignments() const;
  void add_alignment(Count alignment);

private:
  static Score &score_data(PhrasePairData data, DataIndex base, DataIndex index = 0) {
    return *reinterpret_cast<Score *>(data + (base + index) * sizeof(Score));
  }

  static Count &count_data(PhrasePairData data, DataIndex base, DataIndex index = 0) {
    return *reinterpret_cast<Count *>(data + data_nscores_ * sizeof(Score) + (base + index) * sizeof(Count));
  }

  static const Count COUNTS_PER_ALIGNMENT = 2;

  static Count *alignment_data(PhrasePairData data, Count index) {
    return reinterpret_cast<Count *>(data + data_nscores_ * sizeof(Score) + data_ncounts_ * sizeof(Count) + COUNTS_PER_ALIGNMENT * index * sizeof(Count));
  }

  Count *alignment_data(Count index) {
    return alignment_data(data_, index);
  }

  const Count *alignment_data(Count index) const {
    return alignment_data(data_, index);
  }
};

class PhraseTable
{
public:
  typedef PhrasePairInfo value_type;

protected:
  typedef std::iterator_traits<PhrasePairCounts::iterator>::value_type InputEntry_;
  typedef value_type (*EntryTransformer_)(InputEntry_);

  static value_type pass_entry(InputEntry_ in) {
    return PhrasePairInfo(in.first.first, in.first.second, in.second, false);
  }

  static value_type swap_src_tgt(InputEntry_ in) {
    return PhrasePairInfo(in.first.first, in.first.second, in.second, true);
  }

public:
  typedef boost::transform_iterator<EntryTransformer_,PhrasePairCounts::iterator> iterator;
  typedef boost::transform_iterator<EntryTransformer_,PhrasePairCounts::const_iterator> const_iterator;

  virtual ~PhraseTable() {}

  virtual PhraseInfo &get_src_phrase(Phrase src) = 0;
  virtual Count n_src_phrases() const = 0;
  virtual PhraseInfo &get_tgt_phrase(Phrase tgt) = 0;
  virtual Count n_tgt_phrases() const = 0;
  virtual PhrasePairCounts &get_joint_counts() = 0;
  virtual void attach_src_statistic(PhraseStatistic &s) = 0;
  virtual void attach_tgt_statistic(PhraseStatistic &s) = 0;
  virtual void compute_phrase_statistics() = 0;
  virtual DataIndex register_src_data(Count n) = 0;
  virtual DataIndex register_tgt_data(Count n) = 0;
  virtual PhraseTable &reverse() = 0;

  virtual iterator begin() = 0;
  virtual iterator end() = 0;
  virtual iterator find(PhrasePair p) = 0;
  virtual iterator find(const PhrasePairCounts::iterator &it) = 0;

  virtual const_iterator begin() const = 0;
  virtual const_iterator end() const = 0;
  virtual const_iterator find(PhrasePair p) const = 0;
  virtual const_iterator find(const PhrasePairCounts::const_iterator &it) const = 0;

  virtual PhrasePairCounts::iterator raw_begin() = 0;
  virtual PhrasePairCounts::iterator raw_end() = 0;
  virtual PhrasePairCounts::iterator raw_find(PhrasePair p) = 0;

  virtual PhrasePairCounts::const_iterator raw_begin() const = 0;
  virtual PhrasePairCounts::const_iterator raw_end() const = 0;
  virtual PhrasePairCounts::const_iterator raw_find(PhrasePair p) const = 0;

  /*
  	static iterator swap_iterator(const iterator &it) {
  		if(it.functor() == swap_src_tgt)
  			return boost::make_transform_iterator(it.base(), pass_entry);
  		else if(it.functor() == pass_entry)
  			return boost::make_transform_iterator(it.base(), swap_src_tgt);
  		else
  			abort();
  	}

  	static const_iterator swap_iterator(const const_iterator &it) {
  		if(it.functor() == swap_src_tgt)
  			return boost::make_transform_iterator(it.base(), pass_entry);
  		else if(it.functor() == pass_entry)
  			return boost::make_transform_iterator(it.base(), swap_src_tgt);
  		else
  			abort();
  	}
  */
};

class ReversePhraseTable : public PhraseTable
{
protected:
  PhraseTable &phrase_table_;

public:
  ReversePhraseTable(PhraseTable &phrase_table) :
    phrase_table_(phrase_table) {}

  virtual PhraseInfo &get_src_phrase(Phrase src) {
    return phrase_table_.get_tgt_phrase(src);
  }

  virtual Count n_src_phrases() const {
    return phrase_table_.n_tgt_phrases();
  }

  virtual PhraseInfo &get_tgt_phrase(Phrase tgt) {
    return phrase_table_.get_src_phrase(tgt);
  }

  virtual Count n_tgt_phrases() const {
    return phrase_table_.n_src_phrases();
  }

  virtual PhrasePairCounts &get_joint_counts() {
    return phrase_table_.get_joint_counts();
  }

  virtual void attach_src_statistic(PhraseStatistic &s) {
    return phrase_table_.attach_tgt_statistic(s);
  }

  virtual void attach_tgt_statistic(PhraseStatistic &s) {
    return phrase_table_.attach_src_statistic(s);
  }

  virtual void compute_phrase_statistics() {
    phrase_table_.compute_phrase_statistics();
  }

  virtual DataIndex register_src_data(Count n) {
    return phrase_table_.register_tgt_data(n);
  }

  virtual DataIndex register_tgt_data(Count n) {
    return phrase_table_.register_src_data(n);
  }

  virtual PhraseTable &reverse() {
    return phrase_table_;
  }

  virtual iterator begin() {
    return boost::make_transform_iterator(phrase_table_.raw_begin(), swap_src_tgt);
  }

  virtual iterator end() {
    return boost::make_transform_iterator(raw_end(), swap_src_tgt);
  }

  virtual iterator find(PhrasePair p) {
    return boost::make_transform_iterator(raw_find(p), swap_src_tgt);
  }

  virtual iterator find(const PhrasePairCounts::iterator &it) {
    return boost::make_transform_iterator(it, swap_src_tgt);
  }

  virtual const_iterator begin() const {
    return boost::make_transform_iterator(phrase_table_.raw_begin(), swap_src_tgt);
  }

  virtual const_iterator end() const {
    return boost::make_transform_iterator(raw_end(), swap_src_tgt);
  }

  virtual const_iterator find(PhrasePair p) const {
    return boost::make_transform_iterator(raw_find(p), swap_src_tgt);
  }

  virtual const_iterator find(const PhrasePairCounts::const_iterator &it) const {
    return boost::make_transform_iterator(it, swap_src_tgt);
  }

  virtual PhrasePairCounts::iterator raw_begin() {
    return phrase_table_.raw_begin();
  }

  virtual PhrasePairCounts::iterator raw_end() {
    return phrase_table_.raw_end();
  }

  virtual PhrasePairCounts::iterator raw_find(PhrasePair p) {
    return phrase_table_.raw_find(std::make_pair(p.second, p.first));
  }

  virtual PhrasePairCounts::const_iterator raw_begin() const {
    return phrase_table_.raw_begin();
  }

  virtual PhrasePairCounts::const_iterator raw_end() const {
    return phrase_table_.raw_end();
  }

  virtual PhrasePairCounts::const_iterator raw_find(PhrasePair p) const {
    return phrase_table_.raw_find(std::make_pair(p.second, p.first));
  }
};

class MemoryPhraseTable : public PhraseTable
{
protected:
  PhraseInfoList src_info_;
  PhraseInfoList tgt_info_;
  PhrasePairCounts joint_counts_;

  ReversePhraseTable reverse_;

public:
  MemoryPhraseTable() : reverse_(*this) {}

  void load_data(std::istream &instream);

  virtual PhraseInfo &get_src_phrase(Phrase src) {
    assert(src < src_info_.size());
    return src_info_[src];
  }

  virtual Count n_src_phrases() const {
    return src_info_.size();
  }

  virtual PhraseInfo &get_tgt_phrase(Phrase tgt) {
    assert(tgt < tgt_info_.size());
    return tgt_info_[tgt];
  }

  virtual Count n_tgt_phrases() const {
    return tgt_info_.size();
  }

  virtual PhrasePairCounts &get_joint_counts() {
    return joint_counts_;
  }

  virtual void attach_src_statistic(PhraseStatistic &s);
  virtual void attach_tgt_statistic(PhraseStatistic &s);
  virtual void compute_phrase_statistics();

  virtual DataIndex register_src_data(Count n) {
    return src_info_.register_data(n);
  }

  virtual DataIndex register_tgt_data(Count n) {
    return tgt_info_.register_data(n);
  }

  virtual PhraseTable &reverse() {
    return reverse_;
  }

  virtual iterator begin() {
    return boost::make_transform_iterator(raw_begin(), pass_entry);
  }

  virtual iterator end() {
    return boost::make_transform_iterator(raw_end(), pass_entry);
  }

  virtual iterator find(PhrasePair p) {
    return boost::make_transform_iterator(raw_find(p), pass_entry);
  }

  virtual iterator find(const PhrasePairCounts::iterator &it) {
    return boost::make_transform_iterator(it, pass_entry);
  }

  virtual const_iterator begin() const {
    return boost::make_transform_iterator(raw_begin(), pass_entry);
  }

  virtual const_iterator end() const {
    return boost::make_transform_iterator(raw_end(), pass_entry);
  }

  virtual const_iterator find(PhrasePair p) const {
    return boost::make_transform_iterator(raw_find(p), pass_entry);
  }

  virtual const_iterator find(const PhrasePairCounts::const_iterator &it) const {
    return boost::make_transform_iterator(it, pass_entry);
  }

  virtual PhrasePairCounts::iterator raw_begin() {
    return joint_counts_.begin();
  }

  virtual PhrasePairCounts::iterator raw_end() {
    return joint_counts_.end();
  }

  virtual PhrasePairCounts::iterator raw_find(PhrasePair p) {
    return joint_counts_.find(p);
  }

  virtual PhrasePairCounts::const_iterator raw_begin() const {
    return joint_counts_.begin();
  }

  virtual PhrasePairCounts::const_iterator raw_end() const {
    return joint_counts_.end();
  }

  virtual PhrasePairCounts::const_iterator raw_find(PhrasePair p) const {
    return joint_counts_.find(p);
  }
};

#endif
