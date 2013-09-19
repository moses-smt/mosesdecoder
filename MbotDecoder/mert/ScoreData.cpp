/*
 *  ScoreData.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include "ScoreData.h"
#include "Scorer.h"
#include "Util.h"
#include "FileStream.h"

ScoreData::ScoreData(Scorer& ptr):
  theScorer(&ptr)
{
  score_type = theScorer->getName();
  // This is not dangerous: we don't use the this pointer in SetScoreData.
  theScorer->setScoreData(this);
  number_of_scores = theScorer->NumberOfScores();
  // TRACE_ERR("ScoreData: number_of_scores: " << number_of_scores << std::endl);
}

void ScoreData::save(std::ofstream& outFile, bool bin)
{
  for (scoredata_t::iterator i = array_.begin(); i !=array_.end(); i++) {
    i->save(outFile, score_type, bin);
  }
}

void ScoreData::save(const std::string &file, bool bin)
{
  if (file.empty()) return;
  TRACE_ERR("saving the array into " << file << std::endl);

  // matches a stream with a file. Opens the file.
  std::ofstream outFile(file.c_str(), std::ios::out);

  ScoreStats entry;

  save(outFile, bin);

  outFile.close();
}

void ScoreData::load(ifstream& inFile)
{
  ScoreArray entry;

  while (!inFile.eof()) {

    if (!inFile.good()) {
      std::cerr << "ERROR ScoreData::load inFile.good()" << std::endl;
    }

    entry.clear();
    entry.load(inFile);

    if (entry.size() == 0) {
      break;
    }
    add(entry);
  }
}


void ScoreData::load(const std::string &file)
{
  TRACE_ERR("loading score data from " << file << std::endl);

  inputfilestream inFile(file); // matches a stream with a file. Opens the file

  if (!inFile) {
    throw runtime_error("Unable to open score file: " + file);
  }

  load((ifstream&) inFile);

  inFile.close();
}


void ScoreData::add(ScoreArray& e)
{
  if (exists(e.getIndex())) { // array at position e.getIndex() already exists
    //enlarge array at position e.getIndex()
    size_t pos = getIndex(e.getIndex());
    array_.at(pos).merge(e);
  } else {
    array_.push_back(e);
    setIndex();
  }
}

void ScoreData::add(const ScoreStats& e, const std::string& sent_idx)
{
  if (exists(sent_idx)) { // array at position e.getIndex() already exists
    // Enlarge array at position e.getIndex()
    size_t pos = getIndex(sent_idx);
    //          TRACE_ERR("Inserting in array " << sent_idx << std::endl);
    array_.at(pos).add(e);
    //          TRACE_ERR("size: " << size() << " -> " << a.size() << std::endl);
  } else {
    //          TRACE_ERR("Creating a new entry in the array" << std::endl);
    ScoreArray a;
    a.NumberOfScores(number_of_scores);
    a.add(e);
    a.setIndex(sent_idx);
    add(a);
    //          TRACE_ERR("size: " << size() << " -> " << a.size() << std::endl);
  }
}

bool ScoreData::check_consistency() const
{
  if (array_.size() == 0)
    return true;

  for (scoredata_t::const_iterator i = array_.begin(); i != array_.end(); ++i)
    if (!i->check_consistency()) return false;

  return true;
}

void ScoreData::setIndex()
{
  size_t j=0;
  for (scoredata_t::iterator i = array_.begin(); i !=array_.end(); i++) {
    idx2arrayname_[j]=i->getIndex();
    arrayname2idx_[i->getIndex()]=j;
    j++;
  }
}
