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

#ifndef moses_StringVectorTemp_h
#define moses_StringVectorTemp_h

#include <vector>
#include <algorithm>
#include <string>
#include <iterator>
#include <cstdio>
#include <cassert>

#include <boost/iterator/iterator_facade.hpp>

#include "ThrowingFwrite.h"
#include "StringVector.h"

#include "MmapAllocator.h"

namespace Moses
{


// ********** StringVectorTemp **********

template <typename ValueT = unsigned char, typename PosT = unsigned int,
         template <typename> class Allocator = std::allocator>
class StringVectorTemp
{
protected:
  bool m_sorted;
  bool m_memoryMapped;

  std::vector<ValueT, Allocator<ValueT> >* m_charArray;
  std::vector<PosT> m_positions;

  virtual const ValueT* value_ptr(PosT i) const;

public:
  //typedef ValueIteratorRange<typename std::vector<ValueT, Allocator<ValueT> >::const_iterator> range;
  typedef ValueIteratorRange<const ValueT *> range;

  // ********** RangeIterator **********

  class RangeIterator : public boost::iterator_facade<RangeIterator,
    range, std::random_access_iterator_tag, range, PosT>
  {

  private:
    PosT m_index;
    StringVectorTemp<ValueT, PosT, Allocator>* m_container;

  public:
    RangeIterator();
    RangeIterator(StringVectorTemp<ValueT, PosT, Allocator> &sv, PosT index=0);

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
    StringVectorTemp<ValueT, PosT, Allocator>* m_container;

  public:
    StringIterator();
    StringIterator(StringVectorTemp<ValueT, PosT, Allocator> &sv, PosT index=0);

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

  StringVectorTemp();
  StringVectorTemp(Allocator<ValueT> alloc);

  virtual ~StringVectorTemp() {
    delete m_charArray;
  }

  void swap(StringVectorTemp<ValueT, PosT, Allocator> &c) {
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
  //typename std::vector<ValueT, Allocator<ValueT> >::const_iterator begin(PosT i) const;
  //typename std::vector<ValueT, Allocator<ValueT> >::const_iterator end(PosT i) const;
  const ValueT* begin(PosT i) const;
  const ValueT* end(PosT i) const;

  void clear() {
    m_charArray->clear();
    m_sorted = true;
    m_positions.clear();
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
};

// ********** Implementation **********

// StringVectorTemp

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVectorTemp<ValueT, PosT, Allocator>::StringVectorTemp()
  : m_sorted(true), m_memoryMapped(false), m_charArray(new std::vector<ValueT, Allocator<ValueT> >()) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVectorTemp<ValueT, PosT, Allocator>::StringVectorTemp(Allocator<ValueT> alloc)
  : m_sorted(true), m_memoryMapped(false), m_charArray(new std::vector<ValueT, Allocator<ValueT> >(alloc)) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
template <typename StringT>
void StringVectorTemp<ValueT, PosT, Allocator>::push_back(StringT s)
{
  if(is_sorted() && size() && !(back() < s))
    m_sorted = false;

  m_positions.push_back(size2());
  std::copy(s.begin(), s.end(), std::back_inserter(*m_charArray));
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVectorTemp<ValueT, PosT, Allocator>::push_back(const char* c)
{
  std::string dummy(c);
  push_back(dummy);
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
template <typename Iterator>
Iterator StringVectorTemp<ValueT, PosT, Allocator>::begin() const
{
  return Iterator(const_cast<StringVectorTemp<ValueT, PosT, Allocator>&>(*this), 0);
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
template <typename Iterator>
Iterator StringVectorTemp<ValueT, PosT, Allocator>::end() const
{
  return Iterator(const_cast<StringVectorTemp<ValueT, PosT, Allocator>&>(*this), size());
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVectorTemp<ValueT, PosT, Allocator>::iterator StringVectorTemp<ValueT, PosT, Allocator>::begin() const
{
  return begin<iterator>();
};

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVectorTemp<ValueT, PosT, Allocator>::iterator StringVectorTemp<ValueT, PosT, Allocator>::end() const
{
  return end<iterator>();
};

template<typename ValueT, typename PosT, template <typename> class Allocator>
bool StringVectorTemp<ValueT, PosT, Allocator>::is_sorted() const
{
  return m_sorted;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVectorTemp<ValueT, PosT, Allocator>::size() const
{
  return m_positions.size();
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVectorTemp<ValueT, PosT, Allocator>::size2() const
{
  return m_charArray->size();
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVectorTemp<ValueT, PosT, Allocator>::range StringVectorTemp<ValueT, PosT, Allocator>::at(PosT i) const
{
  return range(begin(i), end(i));
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVectorTemp<ValueT, PosT, Allocator>::range StringVectorTemp<ValueT, PosT, Allocator>::operator[](PosT i) const
{
  return at(i);
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVectorTemp<ValueT, PosT, Allocator>::range StringVectorTemp<ValueT, PosT, Allocator>::back() const
{
  return at(size()-1);
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVectorTemp<ValueT, PosT, Allocator>::length(PosT i) const
{
  if(i+1 < size())
    return m_positions[i+1] - m_positions[i];
  else
    return size2() - m_positions[i];
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
const ValueT* StringVectorTemp<ValueT, PosT, Allocator>::value_ptr(PosT i) const
{
  return &(*m_charArray)[m_positions[i]];
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
//typename std::vector<ValueT, Allocator<ValueT> >::const_iterator StringVectorTemp<ValueT, PosT, Allocator>::begin(PosT i) const
const ValueT* StringVectorTemp<ValueT, PosT, Allocator>::begin(PosT i) const
{
  //return typename std::vector<ValueT, Allocator<ValueT> >::const_iterator(value_ptr(i));
  return value_ptr(i);
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
//typename std::vector<ValueT, Allocator<ValueT> >::const_iterator StringVectorTemp<ValueT, PosT, Allocator>::end(PosT i) const
const ValueT* StringVectorTemp<ValueT, PosT, Allocator>::end(PosT i) const
{
  //return typename std::vector<ValueT, Allocator<ValueT> >::const_iterator(value_ptr(i) + length(i));
  return value_ptr(i) + length(i);
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
template <typename StringT>
PosT StringVectorTemp<ValueT, PosT, Allocator>::find(StringT &s) const
{
  if(m_sorted)
    return std::distance(begin(), std::lower_bound(begin(), end(), s));
  return std::distance(begin(), std::find(begin(), end(), s));
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVectorTemp<ValueT, PosT, Allocator>::find(const char* c) const
{
  std::string s(c);
  return find(s);
}

// RangeIterator

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVectorTemp<ValueT, PosT, Allocator>::RangeIterator::RangeIterator() : m_index(0), m_container(0) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVectorTemp<ValueT, PosT, Allocator>::RangeIterator::RangeIterator(StringVectorTemp<ValueT, PosT, Allocator> &sv, PosT index)
  : m_index(index), m_container(&sv) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVectorTemp<ValueT, PosT, Allocator>::RangeIterator::get_index()
{
  return m_index;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
typename StringVectorTemp<ValueT, PosT, Allocator>::range
StringVectorTemp<ValueT, PosT, Allocator>::RangeIterator::dereference() const
{
  return typename StringVectorTemp<ValueT, PosT, Allocator>::range(
           m_container->begin(m_index),
           m_container->end(m_index)
         );
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
bool StringVectorTemp<ValueT, PosT, Allocator>::RangeIterator::equal(
  StringVectorTemp<ValueT, PosT, Allocator>::RangeIterator const& other) const
{
  return m_index == other.m_index && m_container == other.m_container;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVectorTemp<ValueT, PosT, Allocator>::RangeIterator::increment()
{
  m_index++;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVectorTemp<ValueT, PosT, Allocator>::RangeIterator::decrement()
{
  m_index--;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVectorTemp<ValueT, PosT, Allocator>::RangeIterator::advance(PosT n)
{
  m_index += n;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVectorTemp<ValueT, PosT, Allocator>::RangeIterator::distance_to(
  StringVectorTemp<ValueT, PosT, Allocator>::RangeIterator const& other) const
{
  return other.m_index - m_index;
}

// StringIterator

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVectorTemp<ValueT, PosT, Allocator>::StringIterator::StringIterator()
  : m_index(0), m_container(0) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
StringVectorTemp<ValueT, PosT, Allocator>::StringIterator::StringIterator(
  StringVectorTemp<ValueT, PosT, Allocator> &sv, PosT index) : m_index(index),
  m_container(&sv) { }

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVectorTemp<ValueT, PosT, Allocator>::StringIterator::get_index()
{
  return m_index;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
const std::string StringVectorTemp<ValueT, PosT, Allocator>::StringIterator::dereference() const
{
  return StringVectorTemp<ValueT, PosT, Allocator>::range(m_container->begin(m_index),
         m_container->end(m_index)).str();
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
bool StringVectorTemp<ValueT, PosT, Allocator>::StringIterator::equal(
  StringVectorTemp<ValueT, PosT, Allocator>::StringIterator const& other) const
{
  return m_index == other.m_index && m_container == other.m_container;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVectorTemp<ValueT, PosT, Allocator>::StringIterator::increment()
{
  m_index++;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVectorTemp<ValueT, PosT, Allocator>::StringIterator::decrement()
{
  m_index--;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
void StringVectorTemp<ValueT, PosT, Allocator>::StringIterator::advance(PosT n)
{
  m_index += n;
}

template<typename ValueT, typename PosT, template <typename> class Allocator>
PosT StringVectorTemp<ValueT, PosT, Allocator>::StringIterator::distance_to(
  StringVectorTemp<ValueT, PosT, Allocator>::StringIterator const& other) const
{
  return other.m_index - m_index;
}

// ********** Some typedefs **********

typedef StringVectorTemp<unsigned char, unsigned int> MediumStringVectorTemp;
typedef StringVectorTemp<unsigned char, unsigned long> LongStringVectorTemp;

}

#endif
