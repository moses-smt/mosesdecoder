// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (c) 2006 University of Edinburgh
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
			this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
			this list of conditions and the following disclaimer in the documentation
			and/or other materials provided with the distribution.
    * Neither the name of the University of Edinburgh nor the names of its contributors
			may be used to endorse or promote products derived from this software
			without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

// example file on how to use moses library

#pragma once

#include <fstream>
#include <vector>
#include <set>
#include "moses/TypeDef.h"
#include "moses/Sentence.h"
#include "moses/FactorTypeSet.h"
#include "moses/ChartTrellisPathList.h"
#include "moses/OutputCollector.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartTrellisPath.h"
#include "search/applied.hh"
#include "moses/ChartManager.h"

namespace Moses
{
class FactorCollection;
class ChartTrellisPathList;
class ScoreComponentCollection;
}

namespace MosesChartCmd
{

/** Helper class that holds misc variables to write data out to command line.
 */
class IOWrapper
{
protected:
  typedef std::vector<std::pair<Moses::Word, Moses::WordsRange> > ApplicationContext;

  long m_translationId;

  const std::vector<Moses::FactorType>	&m_inputFactorOrder;
  const std::vector<Moses::FactorType>	&m_outputFactorOrder;
  const Moses::FactorMask								&m_inputFactorUsed;
  std::ostream 				        					*m_outputSearchGraphStream;
  std::ostream                          *m_detailedTranslationReportingStream;
  std::ostream                          *m_detailedTreeFragmentsTranslationReportingStream;
  //DIMw
  std::ostream                          *m_detailedAllTranslationReportingStream;
  std::ostream                          *m_alignmentInfoStream;
  std::string		        								m_inputFilePath;
  std::istream					        				*m_inputStream;
  Moses::OutputCollector                *m_detailOutputCollector;
  Moses::OutputCollector                *m_detailTreeFragmentsOutputCollector;
  //DIMw
  Moses::OutputCollector                *m_detailAllOutputCollector;
  Moses::OutputCollector                *m_nBestOutputCollector;
  Moses::OutputCollector                *m_searchGraphOutputCollector;
  Moses::OutputCollector                *m_singleBestOutputCollector;
  Moses::OutputCollector                *m_alignmentInfoCollector;

  typedef std::set< std::pair<size_t, size_t>  > Alignments;
  size_t OutputAlignmentNBest(Alignments &retAlign, const Moses::ChartTrellisNode &node, size_t startTarget);
  size_t OutputAlignment(Alignments &retAlign, const Moses::ChartHypothesis *hypo, size_t startTarget);
  void OutputAlignment(std::vector< std::set<size_t> > &retAlignmentsS2T, const Moses::AlignmentInfo &ai);
  void OutputTranslationOption(std::ostream &out, ApplicationContext &applicationContext, const Moses::ChartHypothesis *hypo, const Moses::Sentence &sentence, long translationId);
  void OutputTranslationOptions(std::ostream &out, ApplicationContext &applicationContext, const Moses::ChartHypothesis *hypo, const Moses::Sentence &sentence, long translationId);
  void OutputTreeFragmentsTranslationOptions(std::ostream &out, ApplicationContext &applicationContext, const Moses::ChartHypothesis *hypo, const Moses::Sentence &sentence, long translationId);
  void ReconstructApplicationContext(const Moses::ChartHypothesis &hypo,
                                     const Moses::Sentence &sentence,
                                     ApplicationContext &context);
  void WriteApplicationContext(std::ostream &out,
                               const ApplicationContext &context);

  void OutputAllFeatureScores(const Moses::ScoreComponentCollection &features
                              , std::ostream &out);
  void OutputFeatureScores( std::ostream& out
                            , const Moses::ScoreComponentCollection &features
                            , const Moses::FeatureFunction *ff
                            , std::string &lastName );

public:
  IOWrapper(const std::vector<Moses::FactorType>	&inputFactorOrder
            , const std::vector<Moses::FactorType>	&outputFactorOrder
            , const Moses::FactorMask							&inputFactorUsed
            , size_t												nBestSize
            , const std::string							&nBestFilePath
            , const std::string							&inputFilePath="");
  ~IOWrapper();

  Moses::InputType* GetInput(Moses::InputType *inputType);
  void OutputBestHypo(const Moses::ChartHypothesis *hypo, long translationId);
  void OutputBestHypo(search::Applied applied, long translationId);
  void OutputBestHypo(const std::vector<const Moses::Factor*>&  mbrBestHypo, long translationId);
  void OutputBestNone(long translationId);
  void OutputNBestList(const Moses::ChartTrellisPathList &nBestList, long translationId);
  void OutputNBestList(const std::vector<search::Applied> &nbest, long translationId);
  void OutputDetailedTranslationReport(const Moses::ChartHypothesis *hypo, const Moses::Sentence &sentence, long translationId);
  void OutputDetailedTreeFragmentsTranslationReport(const Moses::ChartHypothesis *hypo, const Moses::Sentence &sentence, long translationId);
  void OutputDetailedAllTranslationReport(const Moses::ChartTrellisPathList &nBestList, const Moses::ChartManager &manager, const Moses::Sentence &sentence, long translationId);
  void Backtrack(const Moses::ChartHypothesis *hypo);

  void ResetTranslationId();

  Moses::OutputCollector *GetSearchGraphOutputCollector() {
    return m_searchGraphOutputCollector;
  }

  void OutputAlignment(size_t translationId , const Moses::ChartHypothesis *hypo);

  static void FixPrecision(std::ostream &, size_t size=3);
};

}
