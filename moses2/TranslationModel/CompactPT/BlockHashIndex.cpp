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

#include "ThrowingFwrite.h"
#include "BlockHashIndex.h"
#include "CmphStringVectorAdapter.h"
#include "util/exception.hh"
#include "util/string_stream.hh"

#ifdef HAVE_CMPH
#include "cmph.h"
#endif

namespace Moses2
{
#ifdef WITH_THREADS
BlockHashIndex::BlockHashIndex(size_t orderBits, size_t fingerPrintBits,
                               size_t threadsNum) :
  m_orderBits(orderBits), m_fingerPrintBits(fingerPrintBits), m_fileHandle(0), m_fileHandleStart(
    0), m_landmarks(true), m_size(0), m_lastSaved(-1), m_lastDropped(-1), m_numLoadedRanges(
      0), m_threadPool(threadsNum)
{
#ifndef HAVE_CMPH
  std::cerr << "minphr: CMPH support not compiled in." << std::endl;
  exit(1);
#endif
}
#else
BlockHashIndex::BlockHashIndex(size_t orderBits, size_t fingerPrintBits)
  : m_orderBits(orderBits), m_fingerPrintBits(fingerPrintBits),
    m_fileHandle(0), m_fileHandleStart(0), m_size(0),
    m_lastSaved(-1), m_lastDropped(-1), m_numLoadedRanges(0)
{
#ifndef HAVE_CMPH
  std::cerr << "minphr: CMPH support not compiled in." << std::endl;
  exit(1);
#endif
}
#endif

BlockHashIndex::~BlockHashIndex()
{
#ifdef HAVE_CMPH
  for (std::vector<void*>::iterator it = m_hashes.begin(); it != m_hashes.end();
       it++)
    if (*it != 0) cmph_destroy((cmph_t*) *it);

  for (std::vector<PairedPackedArray<>*>::iterator it = m_arrays.begin();
       it != m_arrays.end(); it++)
    if (*it != 0) delete *it;
#endif
}

size_t BlockHashIndex::GetHash(const char* key)
{
  std::string keyStr(key);
  size_t i = std::distance(m_landmarks.begin(),
                           std::upper_bound(m_landmarks.begin(), m_landmarks.end(), keyStr)) - 1;

  if (i == 0ul - 1) return GetSize();

  size_t pos = GetHash(i, key);
  if (pos != GetSize()) return (1ul << m_orderBits) * i + pos;
  else return GetSize();
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
//#ifdef WITH_THREADS
//  boost::mutex::scoped_lock lock(m_mutex);
//#endif
  //if(m_hashes[i] == 0)
  //LoadRange(i);
#ifdef HAVE_CMPH
  size_t idx = cmph_search((cmph_t*) m_hashes[i], key,
                           (cmph_uint32) strlen(key));
#else
  assert(0);
  size_t idx = 0;
#endif

  std::pair<size_t, size_t> orderPrint = m_arrays[i]->Get(idx, m_orderBits,
                                         m_fingerPrintBits);
  m_clocks[i] = clock();

  if (GetFprint(key) == orderPrint.second) return orderPrint.first;
  else return GetSize();
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
  ThrowingFwrite(&m_orderBits, sizeof(size_t), 1, m_fileHandle);
  ThrowingFwrite(&m_fingerPrintBits, sizeof(size_t), 1, m_fileHandle);

  m_fileHandleStart = std::ftell(m_fileHandle);

  size_t relIndexPos = 0;
  ThrowingFwrite(&relIndexPos, sizeof(size_t), 1, m_fileHandle);
}

void BlockHashIndex::SaveRange(size_t i)
{
#ifdef HAVE_CMPH
  if (m_seekIndex.size() <= i) m_seekIndex.resize(i + 1);
  m_seekIndex[i] = std::ftell(m_fileHandle) - m_fileHandleStart;
  cmph_dump((cmph_t*) m_hashes[i], m_fileHandle);
  m_arrays[i]->Save(m_fileHandle);
#endif
}

void BlockHashIndex::SaveLastRange()
{
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_mutex);
#endif

  while (!m_queue.empty() && m_lastSaved + 1 == -m_queue.top()) {
    size_t current = -m_queue.top();
    m_queue.pop();
    SaveRange(current);
    m_lastSaved = current;
  }
}

void BlockHashIndex::DropRange(size_t i)
{
#ifdef HAVE_CMPH
  if (m_hashes[i] != 0) {
    cmph_destroy((cmph_t*) m_hashes[i]);
    m_hashes[i] = 0;
  }
  if (m_arrays[i] != 0) {
    delete m_arrays[i];
    m_arrays[i] = 0;
    m_clocks[i] = 0;
  }
  m_numLoadedRanges--;
#endif
}

void BlockHashIndex::DropLastRange()
{
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_mutex);
#endif

  while (m_lastDropped != m_lastSaved)
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
  ThrowingFwrite(&relIndexPos, sizeof(size_t), 1, m_fileHandle);

  std::fseek(m_fileHandle, m_fileHandleStart + relIndexPos, SEEK_SET);
  m_landmarks.save(m_fileHandle);

  size_t seekIndexSize = m_seekIndex.size();
  ThrowingFwrite(&seekIndexSize, sizeof(size_t), 1, m_fileHandle);
  ThrowingFwrite(&m_seekIndex[0], sizeof(size_t), seekIndexSize, m_fileHandle);

  ThrowingFwrite(&m_size, sizeof(size_t), 1, m_fileHandle);

  size_t fileHandleStop = std::ftell(m_fileHandle);
  return fileHandleStop - m_fileHandleStart + sizeof(m_orderBits)
         + sizeof(m_fingerPrintBits);
}

size_t BlockHashIndex::Save(std::FILE * mphf)
{
  m_queue = std::priority_queue<int>();
  BeginSave(mphf);
  for (size_t i = 0; i < m_hashes.size(); i++)
    SaveRange(i);
  return FinalizeSave();
}

size_t BlockHashIndex::LoadIndex(std::FILE* mphf)
{
  m_fileHandle = mphf;

  size_t beginning = std::ftell(mphf);

  size_t read = 0;
  read += std::fread(&m_orderBits, sizeof(size_t), 1, mphf);
  read += std::fread(&m_fingerPrintBits, sizeof(size_t), 1, mphf);
  m_fileHandleStart = std::ftell(m_fileHandle);

  size_t relIndexPos;
  read += std::fread(&relIndexPos, sizeof(size_t), 1, mphf);
  std::fseek(m_fileHandle, m_fileHandleStart + relIndexPos, SEEK_SET);

  m_landmarks.load(mphf);

  size_t seekIndexSize;
  read += std::fread(&seekIndexSize, sizeof(size_t), 1, m_fileHandle);
  m_seekIndex.resize(seekIndexSize);
  read += std::fread(&m_seekIndex[0], sizeof(size_t), seekIndexSize,
                     m_fileHandle);
  m_hashes.resize(seekIndexSize, 0);
  m_clocks.resize(seekIndexSize, 0);
  m_arrays.resize(seekIndexSize, 0);

  read += std::fread(&m_size, sizeof(size_t), 1, m_fileHandle);

  size_t end = std::ftell(mphf);

  return end - beginning;
}

void BlockHashIndex::LoadRange(size_t i)
{
#ifdef HAVE_CMPH
  std::fseek(m_fileHandle, m_fileHandleStart + m_seekIndex[i], SEEK_SET);
  cmph_t* hash = cmph_load(m_fileHandle);
  m_arrays[i] = new PairedPackedArray<>(0, m_orderBits, m_fingerPrintBits);
  m_arrays[i]->Load(m_fileHandle);

  m_hashes[i] = (void*) hash;
  m_clocks[i] = clock();

  m_numLoadedRanges++;
#endif
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

  for (size_t i = 0; i < m_seekIndex.size(); i++)
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
  /*
   #ifdef WITH_THREADS
   boost::mutex::scoped_lock lock(m_mutex);
   #endif
   size_t n = m_hashes.size() * ratio;
   size_t max = n * (1 + tolerance);
   if(m_numLoadedRanges > max) {
   typedef std::vector<std::pair<clock_t, size_t> > LastLoaded;
   LastLoaded lastLoaded;
   for(size_t i = 0; i < m_hashes.size(); i++)
   if(m_hashes[i] != 0)
   lastLoaded.push_back(std::make_pair(m_clocks[i], i));

   std::sort(lastLoaded.begin(), lastLoaded.end());
   for(LastLoaded::reverse_iterator it = lastLoaded.rbegin() + size_t(n * (1 - tolerance));
   it != lastLoaded.rend(); it++)
   DropRange(it->second);
   }*/
}

void BlockHashIndex::CalcHash(size_t current, void* source_void)
{
#ifdef HAVE_CMPH
  cmph_io_adapter_t* source = (cmph_io_adapter_t*) source_void;
  cmph_config_t *config = cmph_config_new(source);
  cmph_config_set_algo(config, CMPH_CHD);

  cmph_t* hash = cmph_new(config);
  PairedPackedArray<> *pv = new PairedPackedArray<>(source->nkeys, m_orderBits,
      m_fingerPrintBits);

  size_t i = 0;

  source->rewind(source->data);

  std::string lastKey = "";
  while (i < source->nkeys) {
    unsigned keylen;
    char* key;
    source->read(source->data, &key, &keylen);
    std::string temp(key, keylen);
    source->dispose(source->data, key, keylen);

    if (lastKey > temp) {
      if (source->nkeys != 2 || temp != "###DUMMY_KEY###") {
        util::StringStream strme;
        strme
            << "ERROR: Input file does not appear to be sorted with  LC_ALL=C sort\n";
        strme << "1: " << lastKey << "\n";
        strme << "2: " << temp << "\n";
        UTIL_THROW2(strme.str());
      }
    }
    lastKey = temp;

    size_t fprint = GetFprint(temp.c_str());
    size_t idx = cmph_search(hash, temp.c_str(), (cmph_uint32) temp.size());

    pv->Set(idx, i, fprint, m_orderBits, m_fingerPrintBits);
    i++;
  }

  cmph_config_destroy(config);

#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_mutex);
#endif

  if (m_hashes.size() <= current) {
    m_hashes.resize(current + 1, 0);
    m_arrays.resize(current + 1, 0);
    m_clocks.resize(current + 1, 0);
  }

  m_hashes[current] = (void*) hash;
  m_arrays[current] = pv;
  m_clocks[current] = clock();
  m_queue.push(-current);
#endif
}

#ifdef HAVE_CMPH
void* BlockHashIndex::vectorAdapter(std::vector<std::string>& v)
{
  return (void*) CmphVectorAdapter(v);
}

void* BlockHashIndex::vectorAdapter(
  StringVector<unsigned, size_t, std::allocator>& sv)
{
  return (void*) CmphStringVectorAdapter(sv);
}

void* BlockHashIndex::vectorAdapter(
  StringVector<unsigned, size_t, MmapAllocator>& sv)
{
  return (void*) CmphStringVectorAdapter(sv);
}
#endif

}
