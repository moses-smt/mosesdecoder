#ifndef moses_BlockHashIndex_h
#define moses_BlockHashIndex_h

#include <iostream>
#include <string>
#include <vector>
#include <queue> 
#include <cstring>
#include <cstdio>

#include "cmph/src/cmph.h"
#include "MurmurHash3.h"
#include "StringVector.h"
#include "CmphStringVectorAdapter.h"
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
    
    size_t m_orderBits;
    size_t m_fingerPrintBits;

    std::FILE* m_fileHandle;
    size_t m_fileHandleStart;
    
    CMPH_ALGO m_algo;
    
    StringVector<unsigned char, unsigned long> m_landmarks;
    
    std::vector<cmph_t*> m_hashes;
    std::vector<clock_t> m_clocks;
    std::vector<PairedPackedArray<>*> m_arrays;
    
    std::vector<size_t> m_seekIndex;
    
    size_t m_size;
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
    
    size_t GetFprint(const char* key) const;
    size_t GetHash(size_t i, const char* key);
        
  public:
#ifdef WITH_THREADS
    BlockHashIndex(size_t orderBits, size_t fingerPrintBits,
                   size_t threadsNum = 2);
    BlockHashIndex(size_t orderBits, size_t fingerPrintBits, CMPH_ALGO algo,
                   size_t threadsNum = 2);
#else
    BlockHashIndex(size_t orderBits, size_t fingerPrintBits);
    BlockHashIndex(size_t orderBits, size_t fingerPrintBits, CMPH_ALGO algo);
#endif

    ~BlockHashIndex();
        
    size_t GetHash(const char* key);
    size_t GetHash(std::string key);
    
    size_t operator[](std::string key);
    size_t operator[](char* key);
    
    void BeginSave(std::FILE* mphf);
    void SaveRange(size_t i);
    void SaveLastRange();
    size_t FinalizeSave();

#ifdef WITH_THREADS
    void WaitAll();
#endif
 
    void DropRange(size_t i);
    void DropLastRange();
    
    size_t LoadIndex(std::FILE* mphf);
    void LoadRange(size_t i);
    
    size_t Save(std::string filename);
    size_t Save(std::FILE * mphf);
    
    size_t Load(std::string filename);
    size_t Load(std::FILE * mphf);
    
    size_t GetSize() const;
    
    void KeepNLastRanges(float ratio = 0.1, float tolerance = 0.1);
    
    template <typename Keys>
    void AddRange(Keys &keys)
    {
      size_t current = m_landmarks.size();
      m_landmarks.push_back(keys[0]);
      m_size += keys.size();
      
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
      cmph_io_adapter_t *source = VectorAdapter(keys);
          
      cmph_config_t *config = cmph_config_new(source);
      cmph_config_set_algo(config, m_algo);
                
      cmph_t* hash = cmph_new(config);
      cmph_config_destroy(config);
      
      PairedPackedArray<> *pv =
        new PairedPackedArray<>(keys.size(), m_orderBits, m_fingerPrintBits);

      size_t i = 0;
      for(typename Keys::iterator it = keys.begin(); it != keys.end(); it++)
      {
        std::string temp = *it;
        size_t fprint = GetFprint(temp.c_str());
        size_t idx = cmph_search(hash, temp.c_str(),
                                 (cmph_uint32) temp.size());

        pv->Set(idx, i, fprint, m_orderBits, m_fingerPrintBits);
        i++;
      }
      
#ifdef WITH_THREADS
      boost::mutex::scoped_lock lock(m_mutex);
#endif

      if(m_hashes.size() <= current)
      {
        m_hashes.resize(current + 1, 0);    
        m_arrays.resize(current + 1, 0);
        m_clocks.resize(current + 1, 0);
      }
      
      m_hashes[current] = hash;
      m_arrays[current] = pv;
      m_clocks[current] = clock();
      m_queue.push(-current);
    }

    cmph_io_adapter_t* VectorAdapter(std::vector<std::string>& v)
    {
      return CmphVectorAdapter(v);
    }
      
    template <typename ValueT, typename PosT, template <typename> class Allocator>
    cmph_io_adapter_t* VectorAdapter(StringVector<ValueT, PosT, Allocator>& sv)
    {
      return CmphStringVectorAdapter(sv);
    }

};

}
#endif