// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#pragma once

#include <vector>
#include <boost/unordered_map.hpp>
#include "ChartCell.h"
#include "ChartCellCollection.h"
#include "InputType.h"
#include "WordsRange.h"
#include "SentenceStats.h"
#include "ChartTranslationOptionList.h"
#include "ChartParser.h"

#include <boost/shared_ptr.hpp>

namespace Moses
{

class ChartHypothesis;
class ChartTrellisDetourQueue;
class ChartTrellisNode;
class ChartTrellisPath;
class ChartTrellisPathList;

/** Holds everything you need to decode 1 sentence with the hierachical/syntax decoder
 */
class ChartManager
{
private:
  static void CreateDeviantPaths(boost::shared_ptr<const ChartTrellisPath>,
                                 ChartTrellisDetourQueue &);

  static void CreateDeviantPaths(boost::shared_ptr<const ChartTrellisPath>,
                                 const ChartTrellisNode &,
                                 ChartTrellisDetourQueue &);

  InputType const& m_source; /**< source sentence to be translated */
  ChartCellCollection m_hypoStackColl;
  std::auto_ptr<SentenceStats> m_sentenceStats;
  clock_t m_start; /**< starting time, used for logging */
  unsigned m_hypothesisId; /* For handing out hypothesis ids to ChartHypothesis */

  ChartParser m_parser;

  ChartTranslationOptionList m_translationOptionList; /**< pre-computed list of translation options for the phrases in this sentence */

public:
  ChartManager(InputType const& source);
  ~ChartManager();
  void ProcessSentence();
  void AddXmlChartOptions();
  const ChartHypothesis *GetBestHypothesis() const;
  void CalcNBest(size_t count, ChartTrellisPathList &ret, bool onlyDistinct=0) const;

  void GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const;
  void FindReachableHypotheses( const ChartHypothesis *hypo, std::map<unsigned,bool> &reachable ) const; /* auxilliary function for GetSearchGraph */

  //! the input sentence being decoded
  const InputType& GetSource() const {
    return m_source;
  }

  //! debug data collected when decoding sentence
  SentenceStats& GetSentenceStats() const {
    return *m_sentenceStats;
  }

  //DIMw
  const ChartCellCollection& GetChartCellCollection() const {
    return m_hypoStackColl;
  }

  /***
   * to be called after processing a sentence (which may consist of more than just calling ProcessSentence() )
   * currently an empty function
   */
  void CalcDecoderStatistics() const {
  }

  void ResetSentenceStats(const InputType& source) {
    m_sentenceStats = std::auto_ptr<SentenceStats>(new SentenceStats(source));
  }

  //! contigious hypo id for each input sentence. For debugging purposes
  unsigned GetNextHypoId() {
    return m_hypothesisId++;
  }
};

}

