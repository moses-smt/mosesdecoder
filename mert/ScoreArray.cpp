/*
 *  ScoreArray.cpp
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include "ScoreArray.h"
#include "Util.h"
#include "FileStream.h"

using namespace std;

namespace MosesTuning
{


ScoreArray::ScoreArray()
  : m_num_scores(0), m_index(0) {}

void ScoreArray::savetxt(ostream* os, const string& sctype)
{
  *os << SCORES_TXT_BEGIN << " " << m_index << " " << m_array.size()
      << " " << m_num_scores << " " << sctype << endl;
  for (scorearray_t::iterator i = m_array.begin(); i !=m_array.end(); i++) {
    i->savetxt(os);
    *os << endl;
  }
  *os << SCORES_TXT_END << endl;
}

void ScoreArray::savebin(ostream* os, const string& score_type)
{
  *os << SCORES_BIN_BEGIN << " " << m_index << " " << m_array.size()
      << " " << m_num_scores << " " << score_type << endl;
  for (scorearray_t::iterator i = m_array.begin();
       i != m_array.end(); i++) {
    i->savebin(os);
  }
  *os << SCORES_BIN_END << endl;
}

void ScoreArray::save(ostream* os, const string& score_type, bool bin)
{
  if (size() <= 0) return;
  if (bin) {
    savebin(os, score_type);
  } else {
    savetxt(os, score_type);
  }
}

void ScoreArray::save(const string &file, const string& score_type, bool bin)
{
  ofstream ofs(file.c_str(), ios::out);
  if (!ofs) {
    cerr << "Failed to open " << file << endl;
    exit(1);
  }
  ostream* os = &ofs;
  save(os, score_type, bin);
  ofs.close();
}

void ScoreArray::save(const string& score_type, bool bin)
{
  save(&cout, score_type, bin);
}

void ScoreArray::loadbin(istream* is, size_t n)
{
  ScoreStats entry(m_num_scores);
  for (size_t i = 0; i < n; i++) {
    entry.loadbin(is);
    add(entry);
  }
}

void ScoreArray::loadtxt(istream* is, size_t n)
{
  ScoreStats entry(m_num_scores);
  for (size_t i = 0; i < n; i++) {
    entry.loadtxt(is);
    add(entry);
  }
}

void ScoreArray::load(istream* is)
{
  size_t number_of_entries = 0;
  bool binmode = false;

  string substring, stringBuf;
  string::size_type loc;

  getline(*is, stringBuf);
  if (!is->good()) {
    return;
  }

  if (!stringBuf.empty()) {
    if ((loc = stringBuf.find(SCORES_TXT_BEGIN)) == 0) {
      binmode=false;
    } else if ((loc = stringBuf.find(SCORES_BIN_BEGIN)) == 0) {
      binmode=true;
    } else {
      TRACE_ERR("ERROR: ScoreArray::load(): Wrong header");
      return;
    }
    getNextPound(stringBuf, substring);
    getNextPound(stringBuf, substring);
    m_index = atoi(substring.c_str());
    getNextPound(stringBuf, substring);
    number_of_entries = atoi(substring.c_str());
    getNextPound(stringBuf, substring);
    m_num_scores = atoi(substring.c_str());
    getNextPound(stringBuf, substring);
    m_score_type = substring;
  }

  if (binmode) {
    loadbin(is, number_of_entries);
  } else {
    loadtxt(is, number_of_entries);
  }

  getline(*is, stringBuf);
  if (!stringBuf.empty()) {
    if ((loc = stringBuf.find(SCORES_TXT_END)) != 0 &&
        (loc = stringBuf.find(SCORES_BIN_END)) != 0) {
      TRACE_ERR("ERROR: ScoreArray::load(): Wrong footer");
      return;
    }
  }
}

void ScoreArray::load(const string &file)
{
  TRACE_ERR("loading data from " << file << endl);
  inputfilestream input_stream(file); // matches a stream with a file. Opens the file
  istream* is = &input_stream;
  load(is);
  input_stream.close();
}


void ScoreArray::merge(ScoreArray& e)
{
  //dummy implementation
  for (size_t i=0; i<e.size(); i++)
    add(e.get(i));
}

bool ScoreArray::check_consistency() const
{
  const size_t sz = NumberOfScores();
  if (sz == 0)
    return true;

  for (scorearray_t::const_iterator i = m_array.begin();
       i != m_array.end(); ++i) {
    if (i->size() != sz)
      return false;
  }
  return true;
}

}
