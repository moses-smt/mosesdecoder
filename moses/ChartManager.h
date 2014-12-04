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
#include "ChartKBestExtractor.h"
#include "BaseManager.h"
#include "moses/Syntax/KBestExtractor.h"

#include <boost/shared_ptr.hpp>

namespace Moses
{

class ChartHypothesis;
class ChartSearchGraphWriter;

/** Holds everything you need to decode 1 sentence with the hierachical/syntax decoder
 */
class ChartManager : public BaseManager
{
private:
  InputType const& m_source; /**< source sentence to be translated */
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

public:
  ChartManager(InputType const& source);
  ~ChartManager();
  void ProcessSentence();
  void AddXmlChartOptions();
  const ChartHypothesis *GetBestHypothesis() const;
  void CalcNBest(size_t n, std::vector<boost::shared_ptr<ChartKBestExtractor::Derivation> > &nBestList, bool onlyDistinct=false) const;

  /** "Moses" (osg)  type format */
  void OutputSearchGraphMoses(std::ostream &outputSearchGraphStream) const;

  /** Output in (modified) Kenneth hypergraph format */
  void OutputSearchGraphAsHypergraph(std::ostream &outputSearchGraphStream) const;


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

  const ChartParser &GetParser() const { return m_parser; }

  // outputs
  void OutputNBest(OutputCollector *collector) const;
  void OutputLatticeSamples(OutputCollector *collector) const
  {}
  void OutputAlignment(OutputCollector *collector) const;
  void OutputDetailedTranslationReport(OutputCollector *collector) const;
  void OutputUnknowns(OutputCollector *collector) const;
  void OutputDetailedTreeFragmentsTranslationReport(OutputCollector *collector) const;

};

}

