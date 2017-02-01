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

#ifndef moses_PackedArray_h
#define moses_PackedArray_h

#include <vector>
#include <cmath>
#include <cstring>
#include <cstdio>

#include "ThrowingFwrite.h"

namespace Moses2
{

template<typename T = size_t, typename D = unsigned char>
class PackedArray
{
protected:
  static size_t m_dataBits;

  size_t m_size;
  size_t m_storageSize;
  D* m_storage;

public:
  PackedArray() {
    m_size = 0;
    m_storageSize = 0;
    m_storage = new D[0];
  }

  PackedArray(size_t size, size_t bits) :
    m_size(size) {
    m_storageSize = ceil(float(bits * size) / float(m_dataBits));
    m_storage = new D[m_storageSize];
  }

  PackedArray(const PackedArray<T, D> &c) {
    m_size = c.m_size;

    m_storageSize = c.m_storageSize;
    m_storage = new D[m_storageSize];

    std::memcpy(m_storage, c.m_storage, m_storageSize * sizeof(D));
  }

  virtual ~PackedArray() {
    delete[] m_storage;
    m_size = 0;
    m_storageSize = 0;
    m_storage = 0;
  }

  T Get(size_t i, size_t bits) const {
    T out = 0;

    size_t bitstart = (i * bits);
    size_t bitpos = bitstart;

    size_t zero = ((1ul << (bits)) - 1);

    while (bitpos - bitstart < bits) {
      size_t pos = bitpos / m_dataBits;
      size_t off = bitpos % m_dataBits;

      out |= (T(m_storage[pos]) << (bitpos - bitstart)) >> off;

      bitpos += (m_dataBits - off);
    }

    out &= zero;
    return out;
  }

  void Set(size_t i, T v, size_t bits) {
    size_t bitstart = (i * bits);
    size_t bitpos = bitstart;

    while (bitpos - bitstart < bits) {
      size_t pos = bitpos / m_dataBits;
      size_t off = bitpos % m_dataBits;

      size_t rest = bits - (bitpos - bitstart);
      D zero = ~((1ul << (rest + off)) - 1) | ((1ul << off) - 1);

      m_storage[pos] &= zero;
      m_storage[pos] |= v << off;
      v = v >> (m_dataBits - off);
      bitpos += (m_dataBits - off);
    }
  }

  virtual D*& GetStorage() {
    return m_storage;
  }

  virtual size_t GetStorageSize() const {
    return m_storageSize;
  }

  virtual size_t Size() const {
    return m_size;
  }

  virtual size_t Load(std::FILE* in) {
    size_t a1 = std::ftell(in);

    size_t read = 0;
    read += std::fread(&m_size, sizeof(m_size), 1, in);
    read += std::fread(&m_storageSize, sizeof(m_storageSize), 1, in);
    delete[] m_storage;
    m_storage = new D[m_storageSize];
    read += std::fread(m_storage, sizeof(D), m_storageSize, in);

    size_t a2 = std::ftell(in);
    return a2 - a1;
  }

  virtual size_t Save(std::FILE* out) {
    size_t a1 = std::ftell(out);

    ThrowingFwrite(&m_size, sizeof(m_size), 1, out);
    ThrowingFwrite(&m_storageSize, sizeof(m_storageSize), 1, out);
    ThrowingFwrite(m_storage, sizeof(D), m_storageSize, out);

    size_t a2 = std::ftell(out);
    return a2 - a1;
  }

};

template<typename T, typename D>
size_t PackedArray<T, D>::m_dataBits = sizeof(D) * 8;

/**************************************************************************/

template<typename T = size_t, typename D = unsigned char>
class PairedPackedArray: public PackedArray<T, D>
{
public:
  PairedPackedArray() :
    PackedArray<T, D>() {
  }

  PairedPackedArray(size_t size, size_t bits1, size_t bits2) :
    PackedArray<T, D>(size, bits1 + bits2) {
  }

  void Set(size_t i, T a, T b, size_t bits1, size_t bits2) {
    T c = 0;
    c = a | (b << bits1);
    PackedArray<T, D>::Set(i, c, bits1 + bits2);
  }

  void Set(size_t i, std::pair<T, T> p, size_t bits1, size_t bits2) {
    T c = 0;
    c = p.second | (p.first << bits1);
    PackedArray<T, D>::Set(i, c);
  }

  std::pair<T, T> Get(size_t i, size_t bits1, size_t bits2) {
    T v = PackedArray<T, D>::Get(i, bits1 + bits2);
    T a = v & ((1 << bits1) - 1);
    T b = v >> bits1;
    return std::pair<T, T>(a, b);
  }
};

}

#endif
