/*
 *  ScoreData.cpp
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include "ScoreData.h"

#include <iostream>
#include <fstream>
#include "Scorer.h"
#include "Util.h"
#include "FileStream.h"

using namespace std;

namespace MosesTuning
{


ScoreData::ScoreData(Scorer* scorer) :
  m_scorer(scorer)
{
  m_score_type = m_scorer->getName();
  // This is not dangerous: we don't use the this pointer in SetScoreData.
  m_scorer->setScoreData(this);
  m_num_scores = m_scorer->NumberOfScores();
  // TRACE_ERR("ScoreData: m_num_scores: " << m_num_scores << std::endl);
}

void ScoreData::save(ostream* os, bool bin)
{
  for (scoredata_t::iterator i = m_array.begin();
       i != m_array.end(); ++i) {
    i->save(os, m_score_type, bin);
  }
}

void ScoreData::save(const string &file, bool bin)
{
  if (file.empty()) return;
  TRACE_ERR("saving the array into " << file << endl);

  // matches a stream with a file. Opens the file.
  ofstream ofs(file.c_str(), ios::out);
  ostream* os = &ofs;
  save(os, bin);
  ofs.close();
}

void ScoreData::save(bool bin)
{
  save(&cout, bin);
}

void ScoreData::load(istream* is)
{
  ScoreArray entry;

  while (!is->eof()) {
    if (!is->good()) {
      cerr << "ERROR ScoreData::load inFile.good()" << endl;
    }
    entry.clear();
    entry.load(is);
    if (entry.size() == 0) {
      break;
    }
    add(entry);
  }
}

void ScoreData::load(const string &file)
{
  TRACE_ERR("loading score data from " << file << endl);
  inputfilestream input_stream(file); // matches a stream with a file. Opens the file
  if (!input_stream) {
    throw runtime_error("Unable to open score file: " + file);
  }
  istream* is = &input_stream;
  load(is);
  input_stream.close();
}

void ScoreData::add(ScoreArray& e)
{
  if (exists(e.getIndex())) { // array at position e.getIndex() already exists
    //enlarge array at position e.getIndex()
    size_t pos = getIndex(e.getIndex());
    m_array.at(pos).merge(e);
  } else {
    m_array.push_back(e);
    setIndex();
  }
}

void ScoreData::add(const ScoreStats& e, int sent_idx)
{
  if (exists(sent_idx)) { // array at position e.getIndex() already exists
    // Enlarge array at position e.getIndex()
    size_t pos = getIndex(sent_idx);
    //          TRACE_ERR("Inserting in array " << sent_idx << std::endl);
    m_array.at(pos).add(e);
    //          TRACE_ERR("size: " << size() << " -> " << a.size() << std::endl);
  } else {
    //          TRACE_ERR("Creating a new entry in the array" << std::endl);
    ScoreArray a;
    a.NumberOfScores(m_num_scores);
    a.add(e);
    a.setIndex(sent_idx);
    size_t idx = m_array.size();
    m_array.push_back(a);
    m_index_to_array_name[idx] = sent_idx;
    m_array_name_to_index[sent_idx]=idx;
    //          TRACE_ERR("size: " << size() << " -> " << a.size() << std::endl);
  }
}

bool ScoreData::check_consistency() const
{
  if (m_array.size() == 0)
    return true;

  for (scoredata_t::const_iterator i = m_array.begin(); i != m_array.end(); ++i)
    if (!i->check_consistency()) return false;

  return true;
}

void ScoreData::setIndex()
{
  size_t j = 0;
  for (scoredata_t::iterator i = m_array.begin(); i != m_array.end(); ++i) {
    m_index_to_array_name[j] = i->getIndex();
    m_array_name_to_index[i->getIndex()]=j;
    j++;
  }
}

}
