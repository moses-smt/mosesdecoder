// $Id$

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

#ifndef moses_SentenceStats_h
#define moses_SentenceStats_h

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include "Timer.h"
#include "Phrase.h"
#include "Hypothesis.h"
#include "TypeDef.h" //FactorArray
#include "InputType.h"
#include "Util.h" //Join()

namespace Moses
{

//! Hold info about recombination. Used by SentenceStats class
struct RecombinationInfo {
  RecombinationInfo() {} //for std::vector
  RecombinationInfo(size_t srcWords, float gProb, float bProb)
    : numSourceWords(srcWords), betterProb(gProb), worseProb(bProb) {}

  size_t numSourceWords;
  float betterProb, worseProb;
};

/**
 * stats relating to decoder operation on a given sentence
 */
class SentenceStats
{
public:

  /***
   * to be called before decoding a sentence
   */
  SentenceStats(const InputType& source) {
    Initialize(source);
  }
  void Initialize(const InputType& source) {
    m_numHyposCreated = 0;
    m_numHyposPopped = 0;
    m_numHyposPruned = 0;
    m_numHyposDiscarded = 0;
    m_numHyposEarlyDiscarded = 0;
    m_numHyposNotBuilt = 0;
    m_totalSourceWords = source.GetSize();
    m_recombinationInfos.clear();
    m_deletedWords.clear();
    m_insertedWords.clear();
  }

  /***
   * to be called after decoding a sentence
   */
  void CalcFinalStats(const Hypothesis& bestHypo);

  unsigned int GetTotalHypos() const {
    return m_numHyposCreated + m_numHyposNotBuilt;
  }
  unsigned int GetNumHyposPopped() const {
    return m_numHyposPopped;
  }
  size_t GetNumHyposRecombined() const {
    return m_recombinationInfos.size();
  }
  unsigned int GetNumHyposPruned() const {
    return m_numHyposPruned;
  }
  unsigned int GetNumHyposDiscarded() const {
    return m_numHyposDiscarded;
  }
  unsigned int GetNumHyposEarlyDiscarded() const {
    return m_numHyposEarlyDiscarded;
  }
  unsigned int GetNumHyposNotBuilt() const {
    return m_numHyposNotBuilt;
  }
  double GetTimeCollectOpts() const {
    return m_timeCollectOpts.get_elapsed_time();
  }
  double GetTimeBuildHyp() const {
    return m_timeBuildHyp.get_elapsed_time();
  }
  double GetTimeCalcLM() const {
    return m_timeCalcLM.get_elapsed_time();
  }
  double GetTimeOtherScore() const {
    return m_timeOtherScore.get_elapsed_time();
  }
  double GetTimeEstimateScore() const {
    return m_timeEstimateScore.get_elapsed_time();
  }
  double GetTimeStack() const {
    return m_timeStack.get_elapsed_time();
  }
  double GetTimeSetupCubes() const {
    return m_timeSetupCubes.get_elapsed_time();
  }
  double GetTimeManageCubes() const {
    return m_timeManageCubes.get_elapsed_time();
  }
  double GetTimeTotal() const {
    return m_timeTotal.get_elapsed_time();
  }
  size_t GetTotalSourceWords() const {
    return m_totalSourceWords;
  }
  size_t GetNumWordsDeleted() const {
    return m_deletedWords.size();
  }
  size_t GetNumWordsInserted() const {
    return m_insertedWords.size();
  }
  const std::vector<const Phrase*>& GetDeletedWords() const {
    return m_deletedWords;
  }
  const std::vector<std::string>& GetInsertedWords() const {
    return m_insertedWords;
  }

  void AddRecombination(const Hypothesis& worseHypo, const Hypothesis& betterHypo) {
    m_recombinationInfos.push_back(RecombinationInfo(worseHypo.GetWordsBitmap().GetNumWordsCovered(),
                                   betterHypo.GetFutureScore(), worseHypo.GetFutureScore()));
  }
  void AddCreated() {
    m_numHyposCreated++;
  }
  void AddPopped() {
    m_numHyposPopped++;
  }
  void AddPruning() {
    m_numHyposPruned++;
  }
  void AddEarlyDiscarded() {
    m_numHyposEarlyDiscarded++;
  }
  void AddNotBuilt() {
    m_numHyposNotBuilt++;
  }
  void AddDiscarded() {
    m_numHyposDiscarded++;
  }

  void StartTimeCollectOpts() {
    m_timeCollectOpts.start();
  }
  void StopTimeCollectOpts() {
    m_timeCollectOpts.stop();
  }
  void StartTimeBuildHyp() {
    m_timeBuildHyp.start();
  }
  void StopTimeBuildHyp() {
    m_timeBuildHyp.stop();
  }
  void StartTimeCalcLM() {
    m_timeCalcLM.start();
  }
  void StopTimeCalcLM() {
    m_timeCalcLM.stop();
  }
  void StartTimeOtherScore() {
    m_timeOtherScore.start();
  }
  void StopTimeOtherScore() {
    m_timeOtherScore.stop();
  }
  void StartTimeEstimateScore() {
    m_timeEstimateScore.start();
  }
  void StopTimeEstimateScore() {
    m_timeEstimateScore.stop();
  }
  void StartTimeSetupCubes() {
    m_timeSetupCubes.start();
  }
  void StopTimeSetupCubes() {
    m_timeSetupCubes.stop();
  }
  void StartTimeManageCubes() {
    m_timeManageCubes.start();
  }
  void StopTimeManageCubes() {
    m_timeManageCubes.stop();
  }
  void StartTimeStack() {
    m_timeStack.start();
  }
  void StopTimeStack() {
    m_timeStack.stop();
  }
  void StartTimeTotal() {
    m_timeTotal.start();
  }
  void StopTimeTotal() {
    m_timeTotal.stop();
  }

protected:

  /***
   * auxiliary to CalcFinalStats()
   */
  void AddDeletedWords(const Hypothesis& hypo);

  //hypotheses
  // TODO: Move away from clock_t in favor of just storing doubles of the number of seconds
  // since clock seconds aren't reliable in a multi-threaded environment -Jon
  // (see Manager.cpp for some initial work moving in this direction)
  std::vector<RecombinationInfo> m_recombinationInfos;
  unsigned int m_numHyposCreated;
  unsigned int m_numHyposPopped;
  unsigned int m_numHyposPruned;
  unsigned int m_numHyposDiscarded;
  unsigned int m_numHyposEarlyDiscarded;
  unsigned int m_numHyposNotBuilt;
  Timer m_timeCollectOpts;
  Timer m_timeBuildHyp;
  Timer m_timeEstimateScore;
  Timer m_timeOtherScore;
  Timer m_timeCalcLM;
  Timer m_timeStack;
  Timer m_timeSetupCubes;
  Timer m_timeManageCubes;
  Timer m_timeTotal;

  //words
  size_t m_totalSourceWords;
  std::vector<const Phrase*> m_deletedWords; //count deleted words/phrases in the final hypothesis
  std::vector<std::string> m_insertedWords; //count inserted words in the final hypothesis
};

inline std::ostream& operator<<(std::ostream& os, const SentenceStats& ss)
{
  double totalTime = ss.GetTimeTotal();
  double otherTime = totalTime - (ss.GetTimeCollectOpts() + ss.GetTimeBuildHyp() + ss.GetTimeEstimateScore() + ss.GetTimeCalcLM() + ss.GetTimeOtherScore() + ss.GetTimeStack() + ss.GetTimeSetupCubes() + ss.GetTimeManageCubes());

  return os << "total hypotheses considered = " << ss.GetTotalHypos() << std::endl
         << "    number popped from cube = " << ss.GetNumHyposPopped() << std::endl
         << "           number not built = " << ss.GetNumHyposNotBuilt() << std::endl
         << "     number discarded early = " << ss.GetNumHyposEarlyDiscarded() << std::endl
         << "           number discarded = " << ss.GetNumHyposDiscarded() << std::endl
         << "          number recombined = " << ss.GetNumHyposRecombined() << std::endl
         << "              number pruned = " << ss.GetNumHyposPruned() << std::endl

         << "time to collect opts    " << ss.GetTimeCollectOpts()   << " (" << (int)(100 * ss.GetTimeCollectOpts()/totalTime) << "%)" << std::endl
         << "        create hyps     " << ss.GetTimeBuildHyp()      << " (" << (int)(100 * ss.GetTimeBuildHyp()/totalTime) << "%)" << std::endl
         << "        estimate score  " << ss.GetTimeEstimateScore() << " (" << (int)(100 * ss.GetTimeEstimateScore()/totalTime) << "%)" << std::endl
         << "        calc lm         " << ss.GetTimeCalcLM()        << " (" << (int)(100 * ss.GetTimeCalcLM()/totalTime) << "%)" << std::endl
         << "        other hyp score " << ss.GetTimeOtherScore()    << " (" << (int)(100 * ss.GetTimeOtherScore()/totalTime) << "%)" << std::endl
         << "        set up cubes    " << ss.GetTimeSetupCubes()    << " (" << (int)(100 * ss.GetTimeSetupCubes()/totalTime) << "%)" << std::endl
         << "        manage cubes    " << ss.GetTimeManageCubes()   << " (" << (int)(100 * ss.GetTimeManageCubes()/totalTime) << "%)" << std::endl
         << "        manage stacks   " << ss.GetTimeStack()         << " (" << (int)(100 * ss.GetTimeStack()/totalTime) << "%)" << std::endl
         << "        other           " << otherTime                 << " (" << (int)(100 * otherTime/totalTime) << "%)" << std::endl

         << "total source words = " << ss.GetTotalSourceWords() << std::endl
         << "     words deleted = " << ss.GetNumWordsDeleted() << " (" << Join(" ", ss.GetDeletedWords()) << ")" << std::endl
         << "    words inserted = " << ss.GetNumWordsInserted() << " (" << Join(" ", ss.GetInsertedWords()) << ")" << std::endl;
}

}
#endif
