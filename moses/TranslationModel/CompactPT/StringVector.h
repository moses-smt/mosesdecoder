// $Id$
// vim:tabstop=2
/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_StringVector_h
#define moses_StringVector_h

#include <vector>
#include <algorithm>
#include <string>
#include <iterator>
#include <cstdio>
#include <cassert>

#include <boost/iterator/iterator_facade.hpp>

#include "ThrowingFwrite.h"
#include "MonotonicVector.h"
#include "MmapAllocator.h"

namespace Moses
{

// ********** ValueIteratorRange **********

template <typename ValueIteratorT>
class ValueIteratorRange
{
private:
  ValueIteratorT m_begin;
  ValueIteratorT m_end;

public:
  ValueIteratorRange(ValueIteratorT begin, ValueIteratorT end);

  const ValueIteratorT& begin() const;
  const ValueIteratorT& end() const;
  const std::string str() const;
  operator const std::string() {
    return str();
  }

  size_t size() {
    return std::distance(m_begin, m_end);
  }

  template <typename StringT>
  bool operator==(const StringT& o) const;
  bool operator==(const char* c) const;

  template <typename StringT>
  bool operator<(const StringT& o) const;
  bool operator<(const char* c) const;
};

// ********** StringVector **********

template <typename ValueT = unsigned char, typename PosT = unsigned int,
         template <typename> class Allocator = std::allocator>
class StringVector
{
protected:
  bool m_sorted;
  bool m_memoryMapped;

  std::vector<ValueT, Allocator<ValueT> >* m_charArray;
  MonotonicVector<PosT, unsigned int, 32> m_positions;

  virtual const ValueT* value_ptr(PosT i) const;

public:
  typedef ValueIteratorRange<typename std::vector<ValueT, Allocator<ValueT> >::const_iterator> range;

  // ********** RangeIterator **********

  class RangeIterator : public boost::iterator_facade<RangeIterator,
    range, std::random_access_iterator_tag, range, PosT>
  {

  private:
    PosT m_index;
    StringVector<ValueT, PosT, Allocator>* m_container;

  public:
    RangeIterator();
    RangeIterator(StringVector<ValueT, PosT, Allocator> &sv, PosT index=0);

    PosT get_index();

  private:
    friend class boost::iterator_core_access;

    range dereference() const;
    bool equal(RangeIterator const& other) const;
    void increment();
    void decrement();
    void advance(PosT n);

    PosT distance_to(RangeIterator const& other) const;
  };

  // ********** StringIterator **********

  class StringIterator : public boost::iterator_facade<StringIterator,
    std::string, std::random_access_iterator_tag, const std::string, PosT>
  {

  private:
    PosT m_index;
    StringVector<ValueT, PosT, Allocator>* m_container;

  public:
    StringIterator();
    StringIterator(StringVector<ValueT, PosT, Allocator> &sv, PosT index=0);

    PosT get_index();

  private:
    friend class boost::iterator_core_access;

    const std::string dereference() const;
    bool equal(StringIterator const& other) const;
    void increment();
    void decrement();
    void advance(PosT n);
    PosT distance_to(StringIterator const& other) const;
  };

  typedef RangeIterator iterator;
  typedef StringIterator string_iterator;

  StringVector();
  StringVector(Allocator<ValueT> alloc);

  virtual ~StringVector() {
    delete m_charArray;
  }

  void swap(StringVector<ValueT, PosT, Allocator> &c) {
    m_positions.commit();
    m_positions.swap(c.m_positions);
    m_charArray->swap(*c.m_charArray);

    bool temp = m_sorted;
    m_sorted = c.m_sorted;
    c.m_sorted = temp;
  }

  bool is_sorted() const;
  PosT size() const;
  virtual PosT size2() const;

  template<class Iterator> Iterator begin() const;
  template<class Iterator> Iterator end() const;

  iterator begin() const;
  iterator end() const;

  PosT length(PosT i) const;
  typename std::vector<ValueT, Allocator<ValueT> >::const_iterator begin(PosT i) const;
  typename std::vector<ValueT, Allocator<ValueT> >::const_iterator end(PosT i) const;

  void clear() {
    m_charArray->clear();
    m_sorted = true;
    m_positions = MonotonicVector<PosT, unsigned int, 32>();
  }

  range at(PosT i) const;
  range operator[](PosT i) const;
  range back() const;

  template <typename StringT>
  void push_back(StringT s);
  void push_back(const char* c);

  template <typename StringT>
  PosT find(StringT &s) const;
  PosT find(const char* c) const;

  virtual size_t load(std::FILE* in, bool memoryMapped = false) {
    size_t size = 0;
    m_memoryMapped = memoryMapped;

    size += std::fread(&m_sorted, sizeof(bool), 1, in) * sizeof(bool);
    size += m_positions.load(in, m_memoryMapped);

    size += loadCharArray(*m_charArray, in, m_memoryMapped);
    return size;
  }

  size_t loadCharArray(std::vector<ValueT, std::allocator<ValueT> >& c,
                       std::FILE* in, bool map = false) {
    // Can only be read into memory. Mapping not possible with std:allocator.
    assert(map == false);

    size_t byteSize = 0;

    size_t valSize;
    byteSize += std::fread(&valSize, sizeof(size_t), 1, in) * sizeof(size_t);

    c.resize(valSize, 0);
    byteSize += std::fread(&c[0], sizeof(ValueT), valSize, in) * sizeof(ValueT);

    return byteSize;
  }

  size_t loadCharArray(std::vector<ValueT, MmapAllocator<ValueT> >& c,
                       std::FILE* in, bool map = false) {
    size_t byteSize = 0;

    size_t valSize;
    byteSize += std::fread(&valSize, sizeof(size_t), 1, in) * sizeof(size_t);

    if(map == false) {
      // Read data into temporary file (default constructor of MmapAllocator)
      // and map memory onto temporary file. Can be resized.

      c.resize(valSize, 0);
      byteSize += std::fread(&c[0], sizeof(ValueT), valSize, in) * sizeof(ValueT);
    } else {
      // Map it directly on specified region of file "in" starting at valPos
      // with length valSize * sizeof(ValueT). Mapped region cannot be resized.

      size_t valPos = std::ftell(in);
      Allocator<ValueT> alloc(in, valPos);
      std::vector<ValueT, Allocator<ValueT> > charArrayTemp(alloc);
      charArrayTemp.resize(valSize);
      c.swap(charArrayTemp);

      byteSize += valSize * sizeof(ValueT);
    }

    return byteSize;
  }

  size_t load(std::string filename, bool memoryMapped = false) {
    std::FILE* pFile = fopen(filename.c_str(), "r");
    size_t byteSize = load(pFile, memoryMapped);
    fclose(pFile);
    return byteSize;
  }

  size_t save(std::FILE* out) {
    size_t byteSize = 0;
    byteSize += ThrowingFwrite(&m_sorted, sizeof(bool), 1, out) * sizeof(bool);

    byteSize += m_positions.save(out);

    size_t valSize = size2();
    byteSize += ThrowingFwrite(&valSize, sizeof(size_t), 1, out) * sizeof(size_t);
    byteSize += ThrowingFwrite(&(*m_charArray)[0], sizeof(ValueT), valSize, out) * sizeof(ValueT);

    return byteSize;
  }

  size_t save(std::string filename) {
    std::FILE* pFile = fopen(filename.c_str(), "w");
    size_t byteSize = save(pFile);
    fclose(pFile);
    return byteSize;
  }

};

// ********** Implementation **********

// ValueIteratorRange

template <typename ValueIteratorT>
ValueIteratorRange<ValueIteratorT>::ValueIteratorRange(ValueIteratorT begin,
    ValueIteratorT end) : m_begin(begin), m_end(end) { }

template <typename ValueIteratorT>
const ValueIteratorT& ValueIteratorRange<ValueIteratorT>::begin() const
{
  return m_begin;
}

template <typename ValueIteratorT>
const ValueIteratorT& ValueIteratorRange<ValueIteratorT>::end() const
{
  return m_end;
}

template <typename ValueIteratorT>
const std::string ValueIteratorRange<ValueIteratorT>::str() const
{
  std::string dummy;
  for(ValueIteratorT it = m_begin; it != m_end; it++)
    dummy.push_back(*it);
  return dummy;
}

template <typename ValueIteratorT>
template <typename StringT>
bool ValueIteratorRange<ValueIteratorT>::operator==(const StringT& o) const
{
  if(std::distance(m_begin, m_end) == std::distance(o.begin(), o.end()))
    return std::equal(m_begin, m_end, o.begin());
  else
    return false;
}

template <typename ValueIteratorT>
bool ValueIteratorRange<ValueIteratorT>::operator==(const char* c) const
{
  return *this == std::string(c);
}

template <typename ValueIteratorT>
template <typename StringT>
bool ValueIteratorRange<ValueIteratorT>::operator<(const StringT &s2) const
{
  return std::lexicographical_compare(m_begin, m_end, s2.begin(), s2.end(),
                                      std::less<typename std::iterator_traits<ValueIteratorT>::value_type>());
}

template <typename ValueIteratorT>
bool ValueIteratorRange<ValueIteratorT>::operator<(const char* c) const
{
  return *this < std::string(c);
}

template <typename StringT, typename ValueIteratorT>
bool operator<(const StringT &s1, const ValueIteratorRange<ValueIteratorT> &s2)
{
  return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
                                      std::less<typename std::iterator_traits<ValueIteratorT>::value_type>());
}

template <typename ValueIteratorT>
bool operator<(const char* c, const ValueIteratorRange<ValueIteratorT> &s2)
{
  size_t len = std::char_traits<char>::length(c);
  return std::lexicographical_compare(c, c + len, s2.begin(), s2.end(),
                                      std::less<typename std::iterator_traits<ValueIteratorT>::value_type>());
}

template <typename OStream, typename ValueIteratorT>
OStream& operator<<(OStream &os, ValueIteratorRange<ValueIteratorT> cr)
{
  ValueIteratorT it = cr.begin();
  while(it != cr.end())
    os << *(it++);
  return os;
}

// StringVector

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVector<ValueT, PosT, Allocator>::StringVector()
  : m_sorted(true), m_memoryMapped(false), m_charArray(new std::vector<ValueT, Allocator<ValueT> >()) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVector<ValueT, PosT, Allocator>::StringVector(Allocator<ValueT> alloc)
  : m_sorted(true), m_memoryMapped(false), m_charArray(new std::vector<ValueT, Allocator<ValueT> >(alloc)) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
template <typename StringT>
void StringVector<ValueT, PosT, Allocator>::push_back(StringT s)
{
  if(is_sorted() && size() && !(back() < s))
    m_sorted = false;

  m_positions.push_back(size2());
  std::copy(s.begin(), s.end(), std::back_inserter(*m_charArray));
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVector<ValueT, PosT, Allocator>::push_back(const char* c)
{
  std::string dummy(c);
  push_back(dummy);
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
template <typename Iterator>
Iterator StringVector<ValueT, PosT, Allocator>::begin() const
{
  return Iterator(const_cast<StringVector<ValueT, PosT, Allocator>&>(*this), 0);
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
template <typename Iterator>
Iterator StringVector<ValueT, PosT, Allocator>::end() const
{
  return Iterator(const_cast<StringVector<ValueT, PosT, Allocator>&>(*this), size());
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVector<ValueT, PosT, Allocator>::iterator StringVector<ValueT, PosT, Allocator>::begin() const
{
  return begin<iterator>();
};

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVector<ValueT, PosT, Allocator>::iterator StringVector<ValueT, PosT, Allocator>::end() const
{
  return end<iterator>();
};

template<typename ValueT, typename PosT, template <typename> class Allocator>
bool StringVector<ValueT, PosT, Allocator>::is_sorted() const
{
  return m_sorted;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVector<ValueT, PosT, Allocator>::size() const
{
  return m_positions.size();
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVector<ValueT, PosT, Allocator>::size2() const
{
  return m_charArray->size();
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVector<ValueT, PosT, Allocator>::range StringVector<ValueT, PosT, Allocator>::at(PosT i) const
{
  return range(begin(i), end(i));
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVector<ValueT, PosT, Allocator>::range StringVector<ValueT, PosT, Allocator>::operator[](PosT i) const
{
  return at(i);
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVector<ValueT, PosT, Allocator>::range StringVector<ValueT, PosT, Allocator>::back() const
{
  return at(size()-1);
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVector<ValueT, PosT, Allocator>::length(PosT i) const
{
  if(i+1 < size())
    return m_positions[i+1] - m_positions[i];
  else
    return size2() - m_positions[i];
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
const ValueT* StringVector<ValueT, PosT, Allocator>::value_ptr(PosT i) const
{
  return &(*m_charArray)[m_positions[i]];
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename std::vector<ValueT, Allocator<ValueT> >::const_iterator StringVector<ValueT, PosT, Allocator>::begin(PosT i) const
{
  return typename std::vector<ValueT, Allocator<ValueT> >::const_iterator(value_ptr(i));
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename std::vector<ValueT, Allocator<ValueT> >::const_iterator StringVector<ValueT, PosT, Allocator>::end(PosT i) const
{
  return typename std::vector<ValueT, Allocator<ValueT> >::const_iterator(value_ptr(i) + length(i));
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
template <typename StringT>
PosT StringVector<ValueT, PosT, Allocator>::find(StringT &s) const
{
  if(m_sorted)
    return std::distance(begin(), std::lower_bound(begin(), end(), s));
  return std::distance(begin(), std::find(begin(), end(), s));
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVector<ValueT, PosT, Allocator>::find(const char* c) const
{
  std::string s(c);
  return find(s);
}

// RangeIterator

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVector<ValueT, PosT, Allocator>::RangeIterator::RangeIterator() : m_index(0), m_container(0) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVector<ValueT, PosT, Allocator>::RangeIterator::RangeIterator(StringVector<ValueT, PosT, Allocator> &sv, PosT index)
  : m_index(index), m_container(&sv) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVector<ValueT, PosT, Allocator>::RangeIterator::get_index()
{
  return m_index;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVector<ValueT, PosT, Allocator>::range
StringVector<ValueT, PosT, Allocator>::RangeIterator::dereference() const
{
  return typename StringVector<ValueT, PosT, Allocator>::range(
           m_container->begin(m_index),
           m_container->end(m_index)
         );
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
bool StringVector<ValueT, PosT, Allocator>::RangeIterator::equal(
  StringVector<ValueT, PosT, Allocator>::RangeIterator const& other) const
{
  return m_index == other.m_index && m_container == other.m_container;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVector<ValueT, PosT, Allocator>::RangeIterator::increment()
{
  m_index++;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVector<ValueT, PosT, Allocator>::RangeIterator::decrement()
{
  m_index--;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVector<ValueT, PosT, Allocator>::RangeIterator::advance(PosT n)
{
  m_index += n;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVector<ValueT, PosT, Allocator>::RangeIterator::distance_to(
  StringVector<ValueT, PosT, Allocator>::RangeIterator const& other) const
{
  return other.m_index - m_index;
}

// StringIterator

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVector<ValueT, PosT, Allocator>::StringIterator::StringIterator()
  : m_index(0), m_container(0) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVector<ValueT, PosT, Allocator>::StringIterator::StringIterator(
  StringVector<ValueT, PosT, Allocator> &sv, PosT index) : m_index(index),
  m_container(&sv) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVector<ValueT, PosT, Allocator>::StringIterator::get_index()
{
  return m_index;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
const std::string StringVector<ValueT, PosT, Allocator>::StringIterator::dereference() const
{
  return StringVector<ValueT, PosT, Allocator>::range(m_container->begin(m_index),
         m_container->end(m_index)).str();
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
bool StringVector<ValueT, PosT, Allocator>::StringIterator::equal(
  StringVector<ValueT, PosT, Allocator>::StringIterator const& other) const
{
  return m_index == other.m_index && m_container == other.m_container;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVector<ValueT, PosT, Allocator>::StringIterator::increment()
{
  m_index++;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVector<ValueT, PosT, Allocator>::StringIterator::decrement()
{
  m_index--;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVector<ValueT, PosT, Allocator>::StringIterator::advance(PosT n)
{
  m_index += n;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVector<ValueT, PosT, Allocator>::StringIterator::distance_to(
  StringVector<ValueT, PosT, Allocator>::StringIterator const& other) const
{
  return other.m_index - m_index;
}

// ********** Some typedefs **********

typedef StringVector<unsigned char, unsigned int> MediumStringVector;
typedef StringVector<unsigned char, unsigned long> LongStringVector;

}

#endif
