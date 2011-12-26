/*
 *  FeatureStats.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include "Util.h"
#include "ScoreStats.h"

namespace {
const int kAvailableSize = 8;
} // namespace

ScoreStats::ScoreStats()
    : available_(kAvailableSize), entries_(0),
      array_(new ScoreStatsType[available_]) {}

ScoreStats::ScoreStats(const size_t size)
    : available_(size), entries_(size),
      array_(new ScoreStatsType[available_])
{
  memset(array_, 0, GetArraySizeWithBytes());
}

ScoreStats::ScoreStats(std::string &theString)
    : available_(0), entries_(0), array_(NULL)
{
  set(theString);
}

ScoreStats::~ScoreStats()
{
  if (array_) {
    delete [] array_;
    array_ = NULL;
  }
}

void ScoreStats::Copy(const ScoreStats &stats)
{
  available_ = stats.available();
  entries_ = stats.size();
  array_ = new ScoreStatsType[available_];
  memcpy(array_, stats.getArray(), GetArraySizeWithBytes());
}

ScoreStats::ScoreStats(const ScoreStats &stats)
{
  Copy(stats);
}

ScoreStats& ScoreStats::operator=(const ScoreStats &stats)
{
  delete [] array_;
  Copy(stats);
  return *this;
}

void ScoreStats::expand()
{
  available_ *= 2;
  scorestats_t buf = new ScoreStatsType[available_];
  memcpy(buf, array_, GetArraySizeWithBytes());
  delete [] array_;
  array_ = buf;
}

void ScoreStats::add(ScoreStatsType v)
{
  if (isfull()) expand();
  array_[entries_++]=v;
}

void ScoreStats::set(std::string &theString)
{
  std::string substring, stringBuf;
  reset();

  while (!theString.empty()) {
    getNextPound(theString, substring);
    add(ConvertStringToScoreStatsType(substring));
  }
}

void ScoreStats::loadbin(std::ifstream& inFile)
{
  inFile.read((char*)array_, GetArraySizeWithBytes());
}

void ScoreStats::loadtxt(std::ifstream& inFile)
{
  std::string theString;
  std::getline(inFile, theString);
  set(theString);
}

void ScoreStats::loadtxt(const std::string &file)
{
//      TRACE_ERR("loading the stats from " << file << std::endl);

  std::ifstream inFile(file.c_str(), std::ios::in); // matches a stream with a file. Opens the file

  loadtxt(inFile);
}


void ScoreStats::savetxt(const std::string &file)
{
//      TRACE_ERR("saving the stats into " << file << std::endl);

  std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

  savetxt(outFile);
}


void ScoreStats::savetxt(std::ofstream& outFile)
{
  outFile << *this;
}

void ScoreStats::savebin(std::ofstream& outFile)
{
  outFile.write((char*)array_, GetArraySizeWithBytes());
}

ostream& operator<<(ostream& o, const ScoreStats& e)
{
  for (size_t i=0; i< e.size(); i++)
    o << e.get(i) << " ";
  return o;
}

//ADDED_BY_TS
bool operator==(const ScoreStats& s1, const ScoreStats& s2) {
  size_t size = s1.size();

  if (size != s2.size())
    return false;

  for (size_t k=0; k < size; k++) {
    if (s1.get(k) != s2.get(k))
      return false;
  }
  
  return true;
}
//END_ADDED
