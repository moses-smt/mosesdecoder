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
#include "Range.h"
#include "SentenceStats.h"
#include "ChartTranslationOptionList.h"
#include "ChartParser.h"
#include "ChartKBestExtractor.h"
#include "BaseManager.h"
#include "moses/Syntax/KBestExtractor.h"

namespace Moses
{

class ChartHypothesis;
class ChartSearchGraphWriter;

/** Holds everything you need to decode 1 sentence with the hierachical/syntax decoder
 */
class ChartManager : public BaseManager
{
private:
  ChartCellCollection m_hypoStackColl;
  std::auto_ptr<SentenceStats> m_sentenceStats;
  clock_t m_start; /**< starting time, used for logging */
  unsigned m_hypothesisId; /* For handing out hypothesis ids to ChartHypothesis */

  ChartParser m_parser;

  ChartTranslationOptionList m_translationOptionList; /**< pre-computed list of translation options for the phrases in this sentence */

  /* auxilliary functions for SearchGraphs */
  void FindReachableHypotheses(
    const ChartHypothesis *hypo, std::map<unsigned,bool> &reachable , size_t* winners, size_t* losers) const;
  void WriteSearchGraph(const ChartSearchGraphWriter& writer) const;

  // output
  void OutputNBestList(OutputCollector *collector,
                       const ChartKBestExtractor::KBestVec &nBestList,
                       long translationId) const;
  size_t CalcSourceSize(const Moses::ChartHypothesis *hypo) const;
  size_t OutputAlignmentNBest(Alignments &retAlign,
                              const Moses::ChartKBestExtractor::Derivation &derivation,
                              size_t startTarget) const;
  size_t OutputAlignment(Alignments &retAlign,
                         const Moses::ChartHypothesis *hypo,
                         size_t startTarget) const;
  void OutputDetailedTranslationReport(
    OutputCollector *collector,
    const ChartHypothesis *hypo,
    const Sentence &sentence,
    long translationId) const;
  void OutputTranslationOptions(std::ostream &out,
                                ApplicationContext &applicationContext,
                                const ChartHypothesis *hypo,
                                const Sentence &sentence,
                                long translationId) const;
  void OutputTranslationOption(std::ostream &out,
                               ApplicationContext &applicationContext,
                               const ChartHypothesis *hypo,
                               const Sentence &sentence,
                               long translationId) const;
  void ReconstructApplicationContext(const ChartHypothesis &hypo,
                                     const Sentence &sentence,
                                     ApplicationContext &context) const;
  void OutputTreeFragmentsTranslationOptions(std::ostream &out,
      ApplicationContext &applicationContext,
      const ChartHypothesis *hypo,
      const Sentence &sentence,
      long translationId) const;
  void OutputDetailedAllTranslationReport(
    OutputCollector *collector,
    const std::vector<boost::shared_ptr<Moses::ChartKBestExtractor::Derivation> > &nBestList,
    const Sentence &sentence,
    long translationId) const;
  void OutputBestHypo(OutputCollector *collector, const ChartHypothesis *hypo, long translationId) const;
  void Backtrack(const ChartHypothesis *hypo) const;

public:
  ChartManager(ttasksptr const& ttask);
  ~ChartManager();
  void Decode();
  void AddXmlChartOptions();
  const ChartHypothesis *GetBestHypothesis() const;
  void CalcNBest(size_t n, std::vector<boost::shared_ptr<ChartKBestExtractor::Derivation> > &nBestList, bool onlyDistinct=false) const;

  /** "Moses" (osg)  type format */
  void OutputSearchGraphMoses(std::ostream &outputSearchGraphStream) const;

  /** Output in (modified) Kenneth hypergraph format */
  void OutputSearchGraphAsHypergraph(std::ostream &outputSearchGraphStream) const;

  //! debug data collected when decoding sentence
  SentenceStats& GetSentenceStats() const {
    return *m_sentenceStats;
  }

  //DIMw
  const ChartCellCollection& GetChartCellCollection() const {
    return m_hypoStackColl;
  }

  void CalcDecoderStatistics() const {
  }

  void ResetSentenceStats(const InputType& source) {
    m_sentenceStats = std::auto_ptr<SentenceStats>(new SentenceStats(source));
  }

  //! contigious hypo id for each input sentence. For debugging purposes
  unsigned GetNextHypoId() {
    return m_hypothesisId++;
  }

  const ChartParser &GetParser() const {
    return m_parser;
  }

  // outputs
  void OutputBest(OutputCollector *collector) const;
  void OutputNBest(OutputCollector *collector) const;
  void OutputLatticeSamples(OutputCollector *collector) const {
  }
  void OutputAlignment(OutputCollector *collector) const;
  void OutputDetailedTranslationReport(OutputCollector *collector) const;
  void OutputUnknowns(OutputCollector *collector) const;
  void OutputDetailedTreeFragmentsTranslationReport(OutputCollector *collector) const;
  void OutputWordGraph(OutputCollector *collector) const {
  }
  void OutputSearchGraph(OutputCollector *collector) const;
  void OutputSearchGraphSLF() const {
  }
  // void OutputSearchGraphHypergraph() const;

};

}

