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

#ifndef moses_CanonicalHuffman_h
#define moses_CanonicalHuffman_h

#include <string>
#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <boost/unordered_map.hpp>

#include "ThrowingFwrite.h"

namespace Moses
{

template <typename Data>
class CanonicalHuffman
{
private:
  std::vector<Data> m_symbols;
  std::vector<size_t> m_firstCodes;
  std::vector<size_t> m_lengthIndex;

  typedef boost::unordered_map<Data, boost::dynamic_bitset<> > EncodeMap;
  EncodeMap m_encodeMap;

  struct MinHeapSorter {
    std::vector<size_t>& m_vec;

    MinHeapSorter(std::vector<size_t>& vec) : m_vec(vec) { }

    bool operator()(size_t a, size_t b) {
      return m_vec[a] > m_vec[b];
    }
  };

  template <class Iterator>
  void CalcLengths(Iterator begin, Iterator end, std::vector<size_t>& lengths) {
    size_t n = std::distance(begin, end);
    std::vector<size_t> A(2 * n, 0);

    m_symbols.resize(n);
    size_t i = 0;
    for(Iterator it = begin; it != end; it++) {
      m_symbols[i] = it->first;

      A[i] = n + i;
      A[n + i] = it->second;
      i++;
    }

    if(n == 1) {
      lengths.push_back(1);
      return;
    }

    MinHeapSorter hs(A);
    std::make_heap(A.begin(), A.begin() + n, hs);

    size_t h = n;
    size_t m1, m2;
    while(h > 1) {
      m1 = A[0];
      std::pop_heap(A.begin(), A.begin() + h, hs);

      h--;

      m2 = A[0];
      std::pop_heap(A.begin(), A.begin() + h, hs);

      A[h] = A[m1] + A[m2];
      A[h-1] = h;
      A[m1] = A[m2] = h;

      std::push_heap(A.begin(), A.begin() + h, hs);
    }

    A[1] = 0;
    for(size_t i = 2; i < 2*n; i++)
      A[i] = A[A[i]] + 1;

    lengths.resize(n);
    for(size_t i = 0; i < n; i++)
      lengths[i] = A[i + n];
  }

  void CalcCodes(std::vector<size_t>& lengths) {
    std::vector<size_t> numLength;
    for(std::vector<size_t>::iterator it = lengths.begin();
        it != lengths.end(); it++) {
      size_t length = *it;
      if(numLength.size() <= length)
        numLength.resize(length + 1, 0);
      numLength[length]++;
    }

    m_lengthIndex.resize(numLength.size());
    m_lengthIndex[0] = 0;
    for(size_t l = 1; l < numLength.size(); l++)
      m_lengthIndex[l] = m_lengthIndex[l - 1] + numLength[l - 1];

    size_t maxLength = numLength.size() - 1;

    m_firstCodes.resize(maxLength + 1, 0);
    for(size_t l = maxLength - 1; l > 0; l--)
      m_firstCodes[l] = (m_firstCodes[l + 1] + numLength[l + 1]) / 2;

    std::vector<Data> t_symbols;
    t_symbols.resize(lengths.size());

    std::vector<size_t> nextCode = m_firstCodes;
    for(size_t i = 0; i < lengths.size(); i++) {
      Data data = m_symbols[i];
      size_t length = lengths[i];

      size_t pos = m_lengthIndex[length]
                   + (nextCode[length] - m_firstCodes[length]);
      t_symbols[pos] = data;

      nextCode[length] = nextCode[length] + 1;
    }

    m_symbols.swap(t_symbols);
  }

  void CreateCodeMap() {
    for(size_t l = 1; l < m_lengthIndex.size(); l++) {
      size_t intCode = m_firstCodes[l];
      size_t num = ((l+1 < m_lengthIndex.size()) ? m_lengthIndex[l+1]
                    : m_symbols.size()) - m_lengthIndex[l];

      for(size_t i = 0; i < num; i++) {
        Data data = m_symbols[m_lengthIndex[l] + i];
        boost::dynamic_bitset<> bitCode(l, intCode);
        m_encodeMap[data] = bitCode;
        intCode++;
      }
    }
  }

  boost::dynamic_bitset<>& Encode(Data data) {
    return m_encodeMap[data];
  }

  template <class BitWrapper>
  void PutCode(BitWrapper& bitWrapper, boost::dynamic_bitset<>& code) {
    for(int j = code.size()-1; j >= 0; j--)
      bitWrapper.Put(code[j]);
  }

public:

  template <class Iterator>
  CanonicalHuffman(Iterator begin, Iterator end, bool forEncoding = true) {
    std::vector<size_t> lengths;
    CalcLengths(begin, end, lengths);
    CalcCodes(lengths);

    if(forEncoding)
      CreateCodeMap();
  }

  CanonicalHuffman(std::FILE* pFile, bool forEncoding = false) {
    Load(pFile);

    if(forEncoding)
      CreateCodeMap();
  }

  template <class BitWrapper>
  void Put(BitWrapper& bitWrapper, Data data) {
    PutCode(bitWrapper, Encode(data));
  }

  template <class BitWrapper>
  Data Read(BitWrapper& bitWrapper) {
    if(bitWrapper.TellFromEnd()) {
      size_t intCode = bitWrapper.Read();
      size_t len = 1;
      while(intCode < m_firstCodes[len]) {
        intCode = 2 * intCode + bitWrapper.Read();
        len++;
      }
      return m_symbols[m_lengthIndex[len] + (intCode - m_firstCodes[len])];
    }
    return Data();
  }

  size_t Load(std::FILE* pFile) {
    size_t start = std::ftell(pFile);
    size_t read = 0;

    size_t size;
    read += std::fread(&size, sizeof(size_t), 1, pFile);
    m_symbols.resize(size);
    read += std::fread(&m_symbols[0], sizeof(Data), size, pFile);

    read += std::fread(&size, sizeof(size_t), 1, pFile);
    m_firstCodes.resize(size);
    read += std::fread(&m_firstCodes[0], sizeof(size_t), size, pFile);

    read += std::fread(&size, sizeof(size_t), 1, pFile);
    m_lengthIndex.resize(size);
    read += std::fread(&m_lengthIndex[0], sizeof(size_t), size, pFile);

    return std::ftell(pFile) - start;
  }

  size_t Save(std::FILE* pFile) {
    size_t start = std::ftell(pFile);

    size_t size = m_symbols.size();
    ThrowingFwrite(&size, sizeof(size_t), 1, pFile);
    ThrowingFwrite(&m_symbols[0], sizeof(Data), size, pFile);

    size = m_firstCodes.size();
    ThrowingFwrite(&size, sizeof(size_t), 1, pFile);
    ThrowingFwrite(&m_firstCodes[0], sizeof(size_t), size, pFile);

    size = m_lengthIndex.size();
    ThrowingFwrite(&size, sizeof(size_t), 1, pFile);
    ThrowingFwrite(&m_lengthIndex[0], sizeof(size_t), size, pFile);

    return std::ftell(pFile) - start;
  }
};

template <class Container = std::string>
class BitWrapper
{
private:
  Container& m_data;

  typename Container::iterator m_iterator;
  typename Container::value_type m_currentValue;

  size_t m_valueBits;
  typename Container::value_type m_mask;
  size_t m_bitPos;

public:

  BitWrapper(Container &data)
    : m_data(data), m_iterator(m_data.begin()), m_currentValue(0),
      m_valueBits(sizeof(typename Container::value_type) * 8),
      m_mask(1), m_bitPos(0) { }

  bool Read() {
    if(m_bitPos % m_valueBits == 0) {
      if(m_iterator != m_data.end())
        m_currentValue = *m_iterator++;
    } else
      m_currentValue = m_currentValue >> 1;

    m_bitPos++;
    return (m_currentValue & m_mask);
  }

  void Put(bool bit) {
    if(m_bitPos % m_valueBits == 0)
      m_data.push_back(0);

    if(bit)
      m_data[m_data.size()-1] |= m_mask << (m_bitPos % m_valueBits);

    m_bitPos++;
  }

  size_t Tell() {
    return m_bitPos;
  }

  size_t TellFromEnd() {
    if(m_data.size() * m_valueBits < m_bitPos)
      return 0;
    return m_data.size() * m_valueBits - m_bitPos;
  }

  void Seek(size_t bitPos) {
    m_bitPos = bitPos;
    m_iterator = m_data.begin() + int((m_bitPos-1)/m_valueBits);
    m_currentValue = (*m_iterator) >> ((m_bitPos-1) % m_valueBits);
    m_iterator++;
  }

  void SeekFromEnd(size_t bitPosFromEnd) {
    size_t bitPos = m_data.size() * m_valueBits - bitPosFromEnd;
    Seek(bitPos);
  }

  void Reset() {
    m_iterator = m_data.begin();
    m_currentValue = 0;
    m_bitPos = 0;
  }

  Container& GetContainer() {
    return m_data;
  }
};

}

#endif
