#include "BlockHashIndex.h"

namespace Moses
{
#ifdef WITH_THREADS
BlockHashIndex::BlockHashIndex(size_t orderBits, size_t fingerPrintBits,
                               size_t threadsNum)
: m_orderBits(orderBits), m_fingerPrintBits(fingerPrintBits),
  m_fileHandle(0), m_fileHandleStart(0), m_algo(CMPH_CHD), m_size(0),
  m_lastSaved(-1), m_lastDropped(-1), m_numLoadedRanges(0),
  m_threadPool(threadsNum) {}
  
BlockHashIndex::BlockHashIndex(size_t orderBits, size_t fingerPrintBits,
                               CMPH_ALGO algo, size_t threadsNum)
: m_orderBits(orderBits), m_fingerPrintBits(fingerPrintBits),
  m_fileHandle(0), m_fileHandleStart(0), m_algo(algo), m_size(0),
  m_lastSaved(-1), m_lastDropped(-1), m_numLoadedRanges(0),
  m_threadPool(threadsNum) {}
#else
BlockHashIndex::BlockHashIndex(size_t orderBits, size_t fingerPrintBits)
: m_orderBits(orderBits), m_fingerPrintBits(fingerPrintBits),
  m_fileHandle(0), m_fileHandleStart(0), m_algo(CMPH_CHD), m_size(0),
  m_lastSaved(-1), m_lastDropped(-1), m_numLoadedRanges(0) {}

BlockHashIndex::BlockHashIndex(size_t orderBits, size_t fingerPrintBits, CMPH_ALGO algo)
: m_orderBits(orderBits), m_fingerPrintBits(fingerPrintBits),
  m_fileHandle(0), m_fileHandleStart(0), m_algo(algo), m_size(0),
  m_lastSaved(-1), m_lastDropped(-1), m_numLoadedRanges(0) {}
#endif

BlockHashIndex::~BlockHashIndex()
{
  for(std::vector<cmph_t*>::iterator it = m_hashes.begin();
      it != m_hashes.end(); it++)
    if(*it != 0)
      cmph_destroy(*it);
      
  for(std::vector<PairedPackedArray<>*>::iterator it = m_arrays.begin();
      it != m_arrays.end(); it++)
    if(*it != 0)
      delete *it;
}

size_t BlockHashIndex::GetHash(const char* key)
{
  std::string keyStr(key);
  size_t i = std::distance(m_landmarks.begin(),
      std::upper_bound(m_landmarks.begin(),
      m_landmarks.end(), keyStr)) - 1;
  
  if(i == 0ul-1)
    return GetSize();
    
  size_t pos = GetHash(i, key);
  if(pos != GetSize())
    return (1ul << m_orderBits) * i + pos; 
  else
    return GetSize();
}

size_t BlockHashIndex::GetFprint(const char* key) const
{
  size_t hash;
  MurmurHash3_x86_32(key, std::strlen(key), 100000, &hash);
  hash &= (1ul << m_fingerPrintBits) - 1;
  return hash;
}

size_t BlockHashIndex::GetHash(size_t i, const char* key)
{
  if(m_hashes[i] == 0)
    LoadRange(i);
    
  size_t idx = cmph_search(m_hashes[i], key, (cmph_uint32) strlen(key));
  
  std::pair<size_t, size_t> orderPrint = m_arrays[i]->Get(idx, m_orderBits, m_fingerPrintBits);
  m_clocks[i] = clock();
  
  if(GetFprint(key) == orderPrint.second)
      return orderPrint.first;
  else
      return GetSize();
}

size_t BlockHashIndex::GetHash(std::string key)
{
  return GetHash(key.c_str());
}

size_t BlockHashIndex::operator[](std::string key)
{
  return GetHash(key);
}

size_t BlockHashIndex::operator[](char* key)
{
  return GetHash(key);
}

size_t BlockHashIndex::Save(std::string filename)
{
  std::FILE* mphf = std::fopen(filename.c_str(), "w");
  size_t size = Save(mphf);
  std::fclose(mphf);
  return size;
}

void BlockHashIndex::BeginSave(std::FILE * mphf)
{
  m_fileHandle = mphf;
  std::fwrite(&m_orderBits, sizeof(size_t), 1, m_fileHandle);
  std::fwrite(&m_fingerPrintBits, sizeof(size_t), 1, m_fileHandle);
  
  m_fileHandleStart = std::ftell(m_fileHandle);
  
  size_t relIndexPos = 0;
  std::fwrite(&relIndexPos, sizeof(size_t), 1, m_fileHandle);    
}

void BlockHashIndex::SaveRange(size_t i)
{
  if(m_seekIndex.size() <= i)
    m_seekIndex.resize(i+1);
  m_seekIndex[i] = std::ftell(m_fileHandle) - m_fileHandleStart;
  cmph_dump(m_hashes[i], m_fileHandle);
  m_arrays[i]->Save(m_fileHandle);  
}

void BlockHashIndex::SaveLastRange()
{
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_mutex);
#endif

  while(!m_queue.empty() && m_lastSaved + 1 == -m_queue.top())
  {
    size_t current = -m_queue.top();
    m_queue.pop();
    SaveRange(current);
    m_lastSaved = current;
  }   
}

void BlockHashIndex::DropRange(size_t i)
{
  if(m_hashes[i] != 0)
  {
    cmph_destroy(m_hashes[i]);
    m_hashes[i] = 0;
  }
  if(m_arrays[i] != 0)
  {
    delete m_arrays[i];
    m_arrays[i] = 0;
    m_clocks[i] = 0;
  }
  m_numLoadedRanges--;
}

void BlockHashIndex::DropLastRange()
{
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_mutex);
#endif

  while(m_lastDropped != m_lastSaved) 
    DropRange(++m_lastDropped);
}

#ifdef WITH_THREADS
void BlockHashIndex::WaitAll()
{
  m_threadPool.Stop(true);
}
#endif

size_t BlockHashIndex::FinalizeSave()
{
#ifdef WITH_THREADS
  m_threadPool.Stop(true);
#endif

  SaveLastRange();
  
  size_t relIndexPos = std::ftell(m_fileHandle) - m_fileHandleStart;
  
  std::fseek(m_fileHandle, m_fileHandleStart, SEEK_SET);
  std::fwrite(&relIndexPos, sizeof(size_t), 1, m_fileHandle);
  
  std::fseek(m_fileHandle, m_fileHandleStart + relIndexPos, SEEK_SET);
  m_landmarks.save(m_fileHandle);
  
  size_t seekIndexSize = m_seekIndex.size();
  std::fwrite(&seekIndexSize, sizeof(size_t), 1, m_fileHandle);
  std::fwrite(&m_seekIndex[0], sizeof(size_t), seekIndexSize, m_fileHandle);
  
  std::fwrite(&m_size, sizeof(size_t), 1, m_fileHandle);
  
  size_t fileHandleStop = std::ftell(m_fileHandle);
  return fileHandleStop - m_fileHandleStart + sizeof(m_orderBits)
    + sizeof(m_fingerPrintBits);
}

size_t BlockHashIndex::Save(std::FILE * mphf)
{
  m_queue = std::priority_queue<int>();
  BeginSave(mphf);
  for(size_t i = 0; i < m_hashes.size(); i++)
    SaveRange(i);
  return FinalizeSave();
}

size_t BlockHashIndex::LoadIndex(std::FILE* mphf)
{
  m_fileHandle = mphf;
  
  size_t beginning = std::ftell(mphf);

  std::fread(&m_orderBits, sizeof(size_t), 1, mphf);
  std::fread(&m_fingerPrintBits, sizeof(size_t), 1, mphf);
  m_fileHandleStart = std::ftell(m_fileHandle);
  
  size_t relIndexPos;
  std::fread(&relIndexPos, sizeof(size_t), 1, mphf);
  std::fseek(m_fileHandle, m_fileHandleStart + relIndexPos, SEEK_SET);

  m_landmarks.load(mphf);

  size_t seekIndexSize;
  std::fread(&seekIndexSize, sizeof(size_t), 1, m_fileHandle);
  m_seekIndex.resize(seekIndexSize);
  std::fread(&m_seekIndex[0], sizeof(size_t), seekIndexSize, m_fileHandle);
  m_hashes.resize(seekIndexSize, 0);
  m_clocks.resize(seekIndexSize, 0);
  m_arrays.resize(seekIndexSize, 0);
  
  std::fread(&m_size, sizeof(size_t), 1, m_fileHandle);

  size_t end = std::ftell(mphf);

  return end - beginning;  
}

void BlockHashIndex::LoadRange(size_t i)
{
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_mutex);
#endif
  std::fseek(m_fileHandle, m_fileHandleStart + m_seekIndex[i], SEEK_SET);
  cmph_t* hash = cmph_load(m_fileHandle);
  m_arrays[i] = new PairedPackedArray<>(0, m_orderBits,
                                        m_fingerPrintBits);
  m_arrays[i]->Load(m_fileHandle);
  
  m_hashes[i] = hash;
  m_clocks[i] = clock();
  
  m_numLoadedRanges++;
}

size_t BlockHashIndex::Load(std::string filename)
{
  std::FILE* mphf = std::fopen(filename.c_str(), "r");
  size_t size = Load(mphf);
  std::fclose(mphf);
  return size;
}

size_t BlockHashIndex::Load(std::FILE * mphf)
{
  size_t byteSize = LoadIndex(mphf);
  size_t end = std::ftell(mphf);
  
  for(size_t i = 0; i < m_seekIndex.size(); i++)
      LoadRange(i);
  std::fseek(m_fileHandle, end, SEEK_SET);
  return byteSize;
}

size_t BlockHashIndex::GetSize() const
{
  return m_size;
}

void BlockHashIndex::KeepNLastRanges(float ratio, float tolerance)
{
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_mutex);
#endif
  
  size_t n = m_hashes.size() * ratio;
  if(m_numLoadedRanges > size_t(n * (1 + tolerance)))
  {
    typedef std::vector<std::pair<clock_t, size_t> > LastLoaded;
    LastLoaded lastLoaded;
    for(size_t i = 0; i < m_hashes.size(); i++)
      if(m_hashes[i] != 0)
        lastLoaded.push_back(std::make_pair(m_clocks[i], i));
      
    std::sort(lastLoaded.begin(), lastLoaded.end());
    for(LastLoaded::reverse_iterator it = lastLoaded.rbegin() + size_t(n * (1 - tolerance));
        it != lastLoaded.rend(); it++)
      DropRange(it->second);
  }
}

}
