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

#ifndef moses_BlockHashIndex_h
#define moses_BlockHashIndex_h

#include <iostream>
#include <string>
#include <vector>
#include <queue> 
#include <cstring>
#include <cstdio>

#include "MurmurHash3.h"
#include "StringVector.h"
#include "PackedArray.h"

#ifdef WITH_THREADS
#include "ThreadPool.h"
#endif

namespace Moses
{

class BlockHashIndex
{
  private:
    std::priority_queue<int> m_queue;
    
    uint64_t m_orderBits;
    uint64_t m_fingerPrintBits;

    std::FILE* m_fileHandle;
    uint64_t m_fileHandleStart;
        
    StringVector<unsigned char, uint64_t> m_landmarks;
    
    std::vector<void*> m_hashes;
    std::vector<clock_t> m_clocks;
    std::vector<PairedPackedArray<>*> m_arrays;
    
    std::vector<uint64_t> m_seekIndex;
    uint64_t m_size;
    
    int m_lastSaved;
    int m_lastDropped;
    size_t m_numLoadedRanges;
    
#ifdef WITH_THREADS
    ThreadPool m_threadPool;
    boost::mutex m_mutex;
 
    template <typename Keys>
    class HashTask : public Task
    {
      public:
        HashTask(int id, BlockHashIndex& hash, Keys& keys)
        : m_id(id), m_hash(hash), m_keys(new Keys(keys)) {}
        
        virtual void Run()
        {
          m_hash.CalcHash(m_id, *m_keys);
        }
    
        virtual ~HashTask()
        {
            delete m_keys;
        }
    
      private:
        int m_id;
        BlockHashIndex& m_hash;
        Keys* m_keys;
    };
#endif
    
    uint64_t GetFprint(const char* key) const;
    uint64_t GetHash(size_t i, const char* key);
        
  public:
#ifdef WITH_THREADS
    BlockHashIndex(size_t orderBits, size_t fingerPrintBits,
                   size_t threadsNum = 2);
#else
    BlockHashIndex(size_t orderBits, size_t fingerPrintBits);
#endif

    ~BlockHashIndex();
        
    uint64_t GetHash(const char* key);
    uint64_t GetHash(std::string key);
    
    uint64_t operator[](std::string key);
    uint64_t operator[](char* key);
    
    void BeginSave(std::FILE* mphf);
    void SaveRange(size_t i);
    void SaveLastRange();
    uint64_t FinalizeSave();

#ifdef WITH_THREADS
    void WaitAll();
#endif
 
    void DropRange(size_t i);
    void DropLastRange();
    
    uint64_t LoadIndex(std::FILE* mphf);
    void LoadRange(size_t i);
    
    uint64_t Save(std::string filename);
    uint64_t Save(std::FILE * mphf);
    
    uint64_t Load(std::string filename);
    uint64_t Load(std::FILE * mphf);
    
    uint64_t GetSize() const;
    
    void KeepNLastRanges(float ratio = 0.1, float tolerance = 0.1);
    
    template <typename Keys>
    void AddRange(Keys &keys)
    {
      size_t current = m_landmarks.size();
      
      if(m_landmarks.size() && m_landmarks.back().str() >= keys[0])
      {
        std::cerr << "ERROR: Input file does not appear to be sorted with  LC_ALL=C sort" << std::endl;
        std::cerr << "1: " << m_landmarks.back().str() << std::endl;
        std::cerr << "2: " << keys[0] << std::endl;
        abort();
      }
      
      m_landmarks.push_back(keys[0]);
      m_size += keys.size();
      
      if(keys.size() == 1) {
        // add dummy key to avoid null hash
        keys.push_back("###DUMMY_KEY###");
      }
      
#ifdef WITH_THREADS
      HashTask<Keys>* ht = new HashTask<Keys>(current, *this, keys);
      m_threadPool.Submit(ht);
#else
      CalcHash(current, keys);
#endif
    }
    
    template <typename Keys>
    void CalcHash(size_t current, Keys &keys)
    {
#ifdef HAVE_CMPH 
      void* source = vectorAdapter(keys);
      CalcHash(current, source);    
#endif
    }

    void CalcHash(size_t current, void* source);
    
#ifdef HAVE_CMPH 
    void* vectorAdapter(std::vector<std::string>& v);
    void* vectorAdapter(StringVector<unsigned, uint64_t, std::allocator>& sv);
    void* vectorAdapter(StringVector<unsigned, uint64_t, MmapAllocator>& sv);
#endif
};

}
#endif
