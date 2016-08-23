/*
 *  FeatureStats.cpp
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include "Util.h"
#include "ScoreStats.h"
#include <fstream>
#include <iostream>

using namespace std;

namespace
{
const int kAvailableSize = 8;
} // namespace

namespace MosesTuning
{


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
  delete [] m_array;
  m_array = NULL;
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

void ScoreStats::set(const string& str)
{
  reset();
  vector<string> out;
  Tokenize(str.c_str(), ' ', &out);
  for (vector<string>::const_iterator it = out.begin();
       it != out.end(); ++it) {
    add(ConvertStringToScoreStatsType(*it));
  }
}

void ScoreStats::loadbin(istream* is)
{
  is->read(reinterpret_cast<char*>(m_array),
           static_cast<streamsize>(GetArraySizeWithBytes()));
}

void ScoreStats::loadtxt(istream* is)
{
  string line;
  getline(*is, line);
  set(line);
}

void ScoreStats::loadtxt(const string &file)
{
  ifstream ifs(file.c_str(), ios::in); // matches a stream with a file. Opens the file
  if (!ifs) {
    cerr << "Failed to open " << file << endl;
    exit(1);
  }
  istream* is = &ifs;
  loadtxt(is);
}


void ScoreStats::savetxt(const string &file)
{
  ofstream ofs(file.c_str(), ios::out); // matches a stream with a file. Opens the file
  ostream* os = &ofs;
  savetxt(os);
}

void ScoreStats::savetxt(ostream* os)
{
  *os << *this;
}

void ScoreStats::savetxt()
{
  savetxt(&cout);
}

void ScoreStats::savebin(ostream* os)
{
  os->write(reinterpret_cast<char*>(m_array),
            static_cast<streamsize>(GetArraySizeWithBytes()));
}

ostream& operator<<(ostream& o, const ScoreStats& e)
{
  for (size_t i=0; i< e.size(); i++)
    o << e.get(i) << " ";
  return o;
}

bool operator==(const ScoreStats& s1, const ScoreStats& s2)
{
  size_t size = s1.size();

  if (size != s2.size())
    return false;

  for (size_t k=0; k < size; k++) {
    if (s1.get(k) != s2.get(k))
      return false;
  }

  return true;
}

}
