/*
 *  FeatureStats.cpp
 *  mert - Minimum Error Rate Training
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
    : m_available_size(kAvailableSize), m_entries(0),
      m_array(new ScoreStatsType[m_available_size]) {}

ScoreStats::ScoreStats(const size_t size)
    : m_available_size(size), m_entries(size),
      m_array(new ScoreStatsType[m_available_size])
{
  memset(m_array, 0, GetArraySizeWithBytes());
}

ScoreStats::~ScoreStats()
{
  if (m_array) {
    delete [] m_array;
    m_array = NULL;
  }
}

void ScoreStats::Copy(const ScoreStats &stats)
{
  m_available_size = stats.available();
  m_entries = stats.size();
  m_array = new ScoreStatsType[m_available_size];
  memcpy(m_array, stats.getArray(), GetArraySizeWithBytes());
}

ScoreStats::ScoreStats(const ScoreStats &stats)
{
  Copy(stats);
}

ScoreStats& ScoreStats::operator=(const ScoreStats &stats)
{
  delete [] m_array;
  Copy(stats);
  return *this;
}

void ScoreStats::expand()
{
  m_available_size *= 2;
  scorestats_t buf = new ScoreStatsType[m_available_size];
  memcpy(buf, m_array, GetArraySizeWithBytes());
  delete [] m_array;
  m_array = buf;
}

void ScoreStats::add(ScoreStatsType v)
{
  if (isfull()) expand();
  m_array[m_entries++]=v;
}

void ScoreStats::set(const std::string& str)
{
  reset();
  vector<string> out;
  Tokenize(str.c_str(), ' ', &out);
  for (vector<string>::const_iterator it = out.begin();
       it != out.end(); ++it) {
    add(ConvertStringToScoreStatsType(*it));
  }
}

void ScoreStats::loadbin(std::ifstream& inFile)
{
  inFile.read((char*)m_array, GetArraySizeWithBytes());
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
  outFile.write((char*)m_array, GetArraySizeWithBytes());
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
