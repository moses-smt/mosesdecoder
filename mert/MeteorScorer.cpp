#include "MeteorScorer.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <vector>

#include <boost/thread/mutex.hpp>

#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
#include "Fdstream.h"
#endif

#include "ScoreStats.h"
#include "Util.h"

using namespace std;

namespace MosesTuning
{

// Meteor supported
#if (defined(__GLIBCXX__) || defined(__GLIBCPP__)) && !defined(_WIN32)

// for clarity
#define CHILD_STDIN_READ pipefds_input[0]
#define CHILD_STDIN_WRITE pipefds_input[1]
#define CHILD_STDOUT_READ pipefds_output[0]
#define CHILD_STDOUT_WRITE pipefds_output[1]

MeteorScorer::MeteorScorer(const string& config)
  : StatisticsBasedScorer("METEOR",config)
{
  meteor_jar = getConfig("jar", "");
  meteor_lang = getConfig("lang", "en");
  meteor_task = getConfig("task", "tune");
  meteor_m = getConfig("m", "");
  meteor_p = getConfig("p", "");
  meteor_w = getConfig("w", "");
  if (meteor_jar == "") {
    throw runtime_error("Meteor jar required, see MeteorScorer.h for full list of options: --scconfig jar:/path/to/meteor-1.4.jar");
  }
  int pipe_status;
  int pipefds_input[2];
  int pipefds_output[2];
  // Create pipes for process communication
  pipe_status = pipe(pipefds_input);
  if (pipe_status == -1) {
    throw runtime_error("Error creating pipe");
  }
  pipe_status = pipe(pipefds_output);
  if (pipe_status == -1) {
    throw runtime_error("Error creating pipe");
  }
  // Fork
  pid_t pid;
  pid = fork();
  if (pid == pid_t(0)) {
    // Child's IO
    dup2(CHILD_STDIN_READ, 0);
    dup2(CHILD_STDOUT_WRITE, 1);
    close(CHILD_STDIN_WRITE);
    close(CHILD_STDOUT_READ);
    // Call Meteor
    stringstream meteor_cmd;
    meteor_cmd << "java -Xmx1G -jar " << meteor_jar << " - - -stdio -lower -t " << meteor_task << " -l " << meteor_lang;
    if (meteor_m != "") {
      meteor_cmd << " -m '" << meteor_m << "'";
    }
    if (meteor_p != "") {
      meteor_cmd << " -p '" << meteor_p << "'";
    }
    if (meteor_w != "") {
      meteor_cmd << " -w '" << meteor_w << "'";
    }
    TRACE_ERR("Executing: " + meteor_cmd.str() + "\n");
    execl("/bin/bash", "bash", "-c", meteor_cmd.str().c_str(), (char*)NULL);
    throw runtime_error("Continued after execl");
  }
  // Parent's IO
  close(CHILD_STDIN_READ);
  close(CHILD_STDOUT_WRITE);
  m_to_meteor = new ofdstream(CHILD_STDIN_WRITE);
  m_from_meteor = new ifdstream(CHILD_STDOUT_READ);
}

MeteorScorer::~MeteorScorer()
{
  // Cleanup IO
  delete m_to_meteor;
  delete m_from_meteor;
}

void MeteorScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  // Just store strings since we're sending lines to an external process
  for (int incRefs = 0; incRefs < (int)referenceFiles.size(); incRefs++) {
    m_references.clear();
    ifstream in(referenceFiles.at(incRefs).c_str());
    if (!in) {
      throw runtime_error("Unable to open " + referenceFiles.at(incRefs));
    }
    string line;
    while (getline(in, line)) {
      line = this->preprocessSentence(line);
      m_references.push_back(line);
    }
    m_multi_references.push_back(m_references);
  }
  m_references=m_multi_references.at(0);
}

void MeteorScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  string sentence = this->preprocessSentence(text);
  string stats_str;
  stringstream input;
  // SCORE ||| ref1 ||| ref2 ||| ... ||| text
  input << "SCORE";
  for (int incRefs = 0; incRefs < (int)m_multi_references.size(); incRefs++) {
    if (sid >= m_multi_references.at(incRefs).size()) {
      stringstream msg;
      msg << "Sentence id (" << sid << ") not found in reference set";
      throw runtime_error(msg.str());
    }
    string ref = m_multi_references.at(incRefs).at(sid);
    input << " ||| " << ref;
  }
  input << " ||| " << text << "\n";
  // Threadsafe IO
#ifdef WITH_THREADS
  mtx.lock();
#endif
  //TRACE_ERR ( "in: " + input.str() );
  *m_to_meteor << input.str();
  m_from_meteor->getline(stats_str);
  //TRACE_ERR ( "out: " + stats_str + "\n" );
#ifdef WITH_THREADS
  mtx.unlock();
#endif
  entry.set(stats_str);
}

float MeteorScorer::calculateScore(const vector<ScoreStatsType>& comps) const
{
  string score;
  stringstream input;
  // EVAL ||| stats
  input << "EVAL |||";
  copy(comps.begin(), comps.end(), ostream_iterator<int>(input, " "));
  input << "\n";
  // Threadsafe IO
#ifdef WITH_THREADS
  mtx.lock();
#endif
  //TRACE_ERR ( "in: " + input.str() );
  *m_to_meteor << input.str();
  m_from_meteor->getline(score);
  //TRACE_ERR ( "out: " + score + "\n" );
#ifdef WITH_THREADS
  mtx.unlock();
#endif
  return atof(score.c_str());
}

#else

// Meteor unsupported, throw error if used

MeteorScorer::MeteorScorer(const string& config)
  : StatisticsBasedScorer("METEOR",config)
{
  throw runtime_error("Meteor unsupported, requires GLIBCXX");
}

MeteorScorer::~MeteorScorer() {}

void MeteorScorer::setReferenceFiles(const vector<string>& referenceFiles) {}

void MeteorScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry) {}

float MeteorScorer::calculateScore(const vector<ScoreStatsType>& comps) const
{
  // Should never be reached
  return 0.0;
}

#endif

}
