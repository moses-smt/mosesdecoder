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

#ifndef moses_MonotonicVector_h
#define moses_MonotonicVector_h

// MonotonicVector - Represents a monotonic increasing function that maps
// positive integers of any size onto a given number type. Each value has to be
// equal or larger than the previous one. Depending on the stepSize it can save
// up to 90% of memory compared to a std::vector<long>. Time complexity is roughly
// constant, in the worst case, however, stepSize times slower than a normal
// std::vector.

#include <vector>
#include <limits>
#include <algorithm>
#include <cstdio>
#include <cassert>

#include "ThrowingFwrite.h"
#include "ListCoders.h"
#include "MmapAllocator.h"

namespace Moses2
{

template<typename PosT = size_t, typename NumT = size_t, PosT stepSize = 32,
         template<typename > class Allocator = std::allocator>
class MonotonicVector
{
private:
  typedef std::vector<NumT, Allocator<NumT> > Anchors;
  typedef std::vector<unsigned int, Allocator<unsigned int> > Diffs;

  Anchors m_anchors;
  Diffs m_diffs;
  std::vector<unsigned int> m_tempDiffs;

  size_t m_size;
  PosT m_last;
  bool m_final;

public:
  typedef PosT value_type;

  MonotonicVector() :
    m_size(0), m_last(0), m_final(false) {
  }

  size_t size() const {
    return m_size + m_tempDiffs.size();
  }

  PosT at(size_t i) const {
    PosT s = stepSize;
    PosT j = m_anchors[i / s];
    PosT r = i % s;

    typename Diffs::const_iterator it = m_diffs.begin() + j;

    PosT k = 0;
    k += VarInt32::DecodeAndSum(it, m_diffs.end(), 1);
    if (i < m_size) k += Simple9::DecodeAndSum(it, m_diffs.end(), r);
    else if (i < m_size + m_tempDiffs.size()) for (size_t l = 0; l < r; l++)
        k += m_tempDiffs[l];

    return k;
  }

  PosT operator[](PosT i) const {
    return at(i);
  }

  PosT back() const {
    return at(size() - 1);
  }

  void push_back(PosT i) {
    assert(m_final != true);

    if (m_anchors.size() == 0 && m_tempDiffs.size() == 0) {
      m_anchors.push_back(0);
      VarInt32::Encode(&i, &i + 1, std::back_inserter(m_diffs));
      m_last = i;
      m_size++;

      return;
    }

    if (m_tempDiffs.size() == stepSize - 1) {
      Simple9::Encode(m_tempDiffs.begin(), m_tempDiffs.end(),
                      std::back_inserter(m_diffs));
      m_anchors.push_back(m_diffs.size());
      VarInt32::Encode(&i, &i + 1, std::back_inserter(m_diffs));

      m_size += m_tempDiffs.size() + 1;
      m_tempDiffs.clear();
    } else {
      PosT last = m_last;
      PosT diff = i - last;
      m_tempDiffs.push_back(diff);
    }
    m_last = i;
  }

  void commit() {
    assert(m_final != true);
    Simple9::Encode(m_tempDiffs.begin(), m_tempDiffs.end(),
                    std::back_inserter(m_diffs));
    m_size += m_tempDiffs.size();
    m_tempDiffs.clear();
    m_final = true;
  }

  size_t usage() {
    return m_diffs.size() * sizeof(unsigned int)
           + m_anchors.size() * sizeof(NumT);
  }

  size_t load(std::FILE* in, bool map = false) {
    size_t byteSize = 0;

    byteSize += fread(&m_final, sizeof(bool), 1, in) * sizeof(bool);
    byteSize += fread(&m_size, sizeof(size_t), 1, in) * sizeof(size_t);
    byteSize += fread(&m_last, sizeof(PosT), 1, in) * sizeof(PosT);

    byteSize += loadVector(m_diffs, in, map);
    byteSize += loadVector(m_anchors, in, map);

    return byteSize;
  }

  template<typename ValueT>
  size_t loadVector(std::vector<ValueT, std::allocator<ValueT> >& v,
                    std::FILE* in, bool map = false) {
    // Can only be read into memory. Mapping not possible with std:allocator.
    assert(map == false);

    size_t byteSize = 0;

    size_t valSize;
    byteSize += std::fread(&valSize, sizeof(size_t), 1, in) * sizeof(size_t);

    v.resize(valSize, 0);
    byteSize += std::fread(&v[0], sizeof(ValueT), valSize, in) * sizeof(ValueT);

    return byteSize;
  }

  template<typename ValueT>
  size_t loadVector(std::vector<ValueT, MmapAllocator<ValueT> >& v,
                    std::FILE* in, bool map = false) {
    size_t byteSize = 0;

    size_t valSize;
    byteSize += std::fread(&valSize, sizeof(size_t), 1, in) * sizeof(size_t);

    if (map == false) {
      // Read data into temporary file (default constructor of MmapAllocator)
      // and map memory onto temporary file. Can be resized.

      v.resize(valSize, 0);
      byteSize += std::fread(&v[0], sizeof(ValueT), valSize, in)
                  * sizeof(ValueT);
    } else {
      // Map it directly on specified region of file "in" starting at valPos
      // with length valSize * sizeof(ValueT). Mapped region cannot be resized.

      size_t valPos = std::ftell(in);

      Allocator<ValueT> alloc(in, valPos);
      std::vector<ValueT, Allocator<ValueT> > vTemp(alloc);
      vTemp.resize(valSize);
      v.swap(vTemp);

      std::fseek(in, valSize * sizeof(ValueT), SEEK_CUR);
      byteSize += valSize * sizeof(ValueT);
    }

    return byteSize;
  }

  size_t save(std::FILE* out) {
    if (!m_final) commit();

    bool byteSize = 0;
    byteSize += ThrowingFwrite(&m_final, sizeof(bool), 1, out) * sizeof(bool);
    byteSize += ThrowingFwrite(&m_size, sizeof(size_t), 1, out)
                * sizeof(size_t);
    byteSize += ThrowingFwrite(&m_last, sizeof(PosT), 1, out) * sizeof(PosT);

    size_t size = m_diffs.size();
    byteSize += ThrowingFwrite(&size, sizeof(size_t), 1, out) * sizeof(size_t);
    byteSize += ThrowingFwrite(&m_diffs[0], sizeof(unsigned int), size, out)
                * sizeof(unsigned int);

    size = m_anchors.size();
    byteSize += ThrowingFwrite(&size, sizeof(size_t), 1, out) * sizeof(size_t);
    byteSize += ThrowingFwrite(&m_anchors[0], sizeof(NumT), size, out)
                * sizeof(NumT);

    return byteSize;
  }

  void swap(MonotonicVector<PosT, NumT, stepSize, Allocator> &mv) {
    if (!m_final) commit();

    m_diffs.swap(mv.m_diffs);
    m_anchors.swap(mv.m_anchors);
  }
};

}
#endif
