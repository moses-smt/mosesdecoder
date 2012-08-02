#ifndef moses_CanonicalHuffman_h
#define moses_CanonicalHuffman_h

#include <string>
#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <boost/unordered_map.hpp>

namespace Moses {

template<typename PosType, typename DataType> class Hufftree;

template <typename Data, typename Code = size_t>
class CanonicalHuffman
{
  private:
    std::vector<Data> m_symbols;
    
    std::vector<Code> m_firstCodes;
    std::vector<size_t> m_lengthIndex;
    
    typedef boost::unordered_map<Data, boost::dynamic_bitset<> > EncodeMap;
    EncodeMap m_encodeMap;
    
    struct MinHeapSorter {
      std::vector<size_t>& m_vec;
      
      MinHeapSorter(std::vector<size_t>& vec) : m_vec(vec) { }
      
      bool operator()(size_t a, size_t b)
      {
        return m_vec[a] > m_vec[b];
      }
    };
    
    template <class Iterator>
    void CalcLengths(Iterator begin, Iterator end, std::vector<size_t>& lengths)
    {
      size_t n = std::distance(begin, end);
      std::vector<size_t> A(2 * n, 0);
      
      m_symbols.resize(n);
      size_t i = 0;
      for(Iterator it = begin; it != end; it++)
      {
        m_symbols[i] = it->first;
        
        A[i] = n + i;
        A[n + i] = it->second;
        i++;
      }
      
      if(n == 1)
      {
        lengths.push_back(1);
        return;
      }
      
      MinHeapSorter hs(A);
      std::make_heap(A.begin(), A.begin() + n, hs);
      
      size_t h = n;
      size_t m1, m2;
      while(h > 1)
      {
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


    void CalcCodes(std::vector<size_t>& lengths)
    {
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
      for(size_t i = 0; i < lengths.size(); i++)
      {    
        Data data = m_symbols[i];
        size_t length = lengths[i];
        
        size_t pos = m_lengthIndex[length]
                     + (nextCode[length] - m_firstCodes[length]);
        t_symbols[pos] = data;

        nextCode[length] = nextCode[length] + 1;
      }
      
      m_symbols.swap(t_symbols);
    }
    
  public:

    CanonicalHuffman(std::FILE* pFile, bool forEncoding = false)
    {
      Load(pFile);
      
      if(forEncoding)
        CreateCodeMap();
    }
    
    template <class Iterator>
    CanonicalHuffman(Iterator begin, Iterator end, bool forEncoding = true)
    {
      std::vector<size_t> lengths;
      CalcLengths(begin, end, lengths);
      CalcCodes(lengths);

      if(forEncoding)
        CreateCodeMap();
    }
    
    void CreateCodeMap()
    {
      for(size_t l = 1; l < m_lengthIndex.size(); l++)
      {
        Code code = m_firstCodes[l];
        size_t num = ((l+1 < m_lengthIndex.size()) ? m_lengthIndex[l+1]
                      : m_symbols.size()) - m_lengthIndex[l];
        
        for(size_t i = 0; i < num; i++)
        {
          Data data = m_symbols[m_lengthIndex[l] + i];  
          boost::dynamic_bitset<> bitCode(l, code);
          m_encodeMap[data] = bitCode;  
          code++;
        }
      }
    }
    
    boost::dynamic_bitset<>& Encode(Data data)
    {
      return m_encodeMap[data];
    }
    
    template <class BitStream>
    Data NextSymbol(BitStream& bitStream)
    {
      if(bitStream.RemainingBits())
      {
        Code code = bitStream.GetNext();
        size_t length = 1;
        while(code < m_firstCodes[length])
        {
          code = 2 * code + bitStream.GetNext();
          length++;
        }
        
        size_t symbolIndex = m_lengthIndex[length]
                             + (code - m_firstCodes[length]);  
        return m_symbols[symbolIndex];
      }   
      return Data();
    }
    
    size_t Load(std::FILE* pFile)
    {
      size_t start = std::ftell(pFile);
      
      size_t size;
      std::fread(&size, sizeof(size_t), 1, pFile);
      m_symbols.resize(size);
      std::fread(&m_symbols[0], sizeof(Data), size, pFile);
      
      std::fread(&size, sizeof(size_t), 1, pFile);
      m_firstCodes.resize(size);
      std::fread(&m_firstCodes[0], sizeof(Code), size, pFile);
      
      std::fread(&size, sizeof(size_t), 1, pFile);
      m_lengthIndex.resize(size);
      std::fread(&m_lengthIndex[0], sizeof(size_t), size, pFile);
      
      return std::ftell(pFile) - start;
    }
    
    size_t Save(std::FILE* pFile)
    {
      size_t start = std::ftell(pFile);
      
      size_t size = m_symbols.size();
      std::fwrite(&size, sizeof(size_t), 1, pFile);
      std::fwrite(&m_symbols[0], sizeof(Data), size, pFile);
      
      size = m_firstCodes.size();
      std::fwrite(&size, sizeof(size_t), 1, pFile);
      std::fwrite(&m_firstCodes[0], sizeof(Code), size, pFile);
      
      size = m_lengthIndex.size();
      std::fwrite(&size, sizeof(size_t), 1, pFile);
      std::fwrite(&m_lengthIndex[0], sizeof(size_t), size, pFile);
      
      return std::ftell(pFile) - start;
    }
    
};

template <class Container = std::string>
class BitStream
{
  private:
    Container& m_data;
    
    typename Container::iterator m_iterator;
    typename Container::value_type m_currentValue;
    
    size_t m_valueBits;
    typename Container::value_type m_mask;
    size_t m_bitPos;
    
  public:
    
    BitStream(Container &data)
    : m_data(data), m_iterator(m_data.begin()),
      m_valueBits(sizeof(typename Container::value_type) * 8),
      m_mask(1), m_bitPos(0) { }
    
    size_t RemainingBits()
    {
      if(m_data.size() * m_valueBits < m_bitPos)
        return 0;
      return m_data.size() * m_valueBits - m_bitPos;
    }
    
    void SetLeft(size_t bitPos)
    {
      m_bitPos = m_data.size() * m_valueBits - bitPos;
      m_iterator = m_data.begin() + int((m_bitPos-1)/m_valueBits);
      m_currentValue = (*m_iterator) >> ((m_bitPos-1) % m_valueBits);
      m_iterator++;
    }
    
    bool GetNext()
    {
      if(m_bitPos % m_valueBits == 0)
      {
        if(m_iterator != m_data.end())
        {
          m_currentValue = *m_iterator++;
        }
      }
      else
      {
        m_currentValue = m_currentValue >> 1;
      } 
      
      m_bitPos++;
      return (m_currentValue & m_mask);
    }
    
    void PutCode(boost::dynamic_bitset<> code)
    {
      
      for(int j = code.size()-1; j >= 0; j--)
      {    
        if(m_bitPos % m_valueBits == 0)
        {
          m_data.push_back(0);
        }
        
        if(code[j])
          m_data[m_data.size()-1] |= m_mask << (m_bitPos % m_valueBits);
      
        m_bitPos++;
      }
      
    }
    
    void Reset()
    {
      m_iterator = m_data.begin();
      m_bitPos = 0;
    }
    
    Container& GetContainer()
    {
      return m_data;
    }
};

}

#endif