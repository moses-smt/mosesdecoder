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

#include <cassert>
#include <fstream>
#include <ostream>
#include <vector>

#include "moses/TypeDef.h"
#include "moses/Sentence.h"
#include "moses/FactorTypeSet.h"
#include "moses/FactorCollection.h"
#include "moses/Hypothesis.h"
#include "moses/OutputCollector.h"
#include "moses/TrellisPathList.h"
#include "moses/InputFileStream.h"
#include "moses/InputType.h"
#include "moses/WordLattice.h"
#include "moses/LatticeMBR.h"
#include "moses/ChartKBestExtractor.h"
#include "moses/Syntax/KBestExtractor.h"

#include "search/applied.hh"

namespace Moses
{
class ScoreComponentCollection;
class Hypothesis;
class ChartHypothesis;
class Factor;

namespace Syntax
{
struct SHyperedge;
}

/** Helper class that holds misc variables to write data out to command line.
 */
class IOWrapper
{
protected:

  const std::vector<Moses::FactorType>	&m_inputFactorOrder;
  const std::vector<Moses::FactorType>	&m_outputFactorOrder;
  const Moses::FactorMask							&m_inputFactorUsed;
  std::string										m_inputFilePath;
  Moses::InputFileStream				*m_inputFile;
  std::istream									*m_inputStream;
  std::ostream *m_nBestStream;
  std::ostream *m_outputWordGraphStream;
  std::ostream *m_detailedTranslationReportingStream;
  std::ofstream *m_alignmentInfoStream;
  std::ostream  *m_unknownsStream;
  std::ostream  *m_outputSearchGraphStream;
  std::ofstream *m_latticeSamplesStream;
  std::ostream *m_detailedTreeFragmentsTranslationReportingStream;

  bool m_surpressSingleBestOutput;

  Moses::OutputCollector *m_singleBestOutputCollector;
  Moses::OutputCollector *m_nBestOutputCollector;
  Moses::OutputCollector *m_unknownsCollector;
  Moses::OutputCollector *m_alignmentInfoCollector;
  Moses::OutputCollector *m_searchGraphOutputCollector;
  Moses::OutputCollector *m_detailedTranslationCollector;
  Moses::OutputCollector *m_wordGraphCollector;
  Moses::OutputCollector *m_latticeSamplesCollector;
  Moses::OutputCollector *m_detailTreeFragmentsOutputCollector;

  // CHART
  typedef std::vector<std::pair<Moses::Word, Moses::WordsRange> > ApplicationContext;
  typedef std::set< std::pair<size_t, size_t>  > Alignments;

  void Backtrack(const ChartHypothesis *hypo);
  void OutputTranslationOptions(std::ostream &out, ApplicationContext &applicationContext, const Moses::ChartHypothesis *hypo, const Moses::Sentence &sentence, long translationId);
  void OutputTranslationOptions(std::ostream &out, ApplicationContext &applicationContext, const search::Applied *applied, const Moses::Sentence &sentence, long translationId);
  void OutputTranslationOption(std::ostream &out, ApplicationContext &applicationContext, const Moses::ChartHypothesis *hypo, const Moses::Sentence &sentence, long translationId);
  void OutputTranslationOption(std::ostream &out, ApplicationContext &applicationContext, const search::Applied *applied, const Moses::Sentence &sentence, long translationId);

  void ReconstructApplicationContext(const Moses::ChartHypothesis &hypo,
                                     const Moses::Sentence &sentence,
                                     ApplicationContext &context);
  void ReconstructApplicationContext(const search::Applied *applied,
                                     const Moses::Sentence &sentence,
                                     ApplicationContext &context);
  void WriteApplicationContext(std::ostream &out,
                               const ApplicationContext &context);
  void OutputTreeFragmentsTranslationOptions(std::ostream &out,
		  	  	  	  	  	  ApplicationContext &applicationContext,
		  	  	  	  	  	  const Moses::ChartHypothesis *hypo,
		  	  	  	  	  	  const Moses::Sentence &sentence,
		  	  	  	  	  	  long translationId);
  void OutputTreeFragmentsTranslationOptions(std::ostream &out,
		  	  	  	  	  	  ApplicationContext &applicationContext,
		  	  	  	  	  	  const search::Applied *applied,
		  	  	  	  	  	  const Moses::Sentence &sentence,
		  	  	  	  	  	  long translationId);

  void OutputSurface(std::ostream &out, const Phrase &phrase, const std::vector<FactorType> &outputFactorOrder, bool reportAllFactors);
  void OutputSurface(std::ostream &out, const Hypothesis &edge, const std::vector<FactorType> &outputFactorOrder,
                     char reportSegmentation, bool reportAllFactors);

  size_t OutputAlignment(Alignments &retAlign, const Moses::ChartHypothesis *hypo, size_t startTarget);
  size_t OutputAlignmentNBest(Alignments &retAlign,
		  	  	  	  	  	  const Moses::ChartKBestExtractor::Derivation &derivation,
		  	  	  	  	  	  size_t startTarget);
  std::size_t OutputAlignmentNBest(Alignments &retAlign, const Moses::Syntax::KBestExtractor::Derivation &derivation, std::size_t startTarget);

  size_t CalcSourceSize(const Moses::ChartHypothesis *hypo);
  size_t CalcSourceSize(const Syntax::KBestExtractor::Derivation &d) const;

  template <class T>
  void ShiftOffsets(std::vector<T> &offsets, T shift)
  {
    T currPos = shift;
    for (size_t i = 0; i < offsets.size(); ++i) {
      if (offsets[i] == 0) {
        offsets[i] = currPos;
        ++currPos;
      } else {
        currPos += offsets[i];
      }
    }
  }

public:
  static IOWrapper *GetIOWrapper(const Moses::StaticData &staticData);
  static void FixPrecision(std::ostream &, size_t size=3);

  IOWrapper(const std::vector<Moses::FactorType>	&inputFactorOrder
            , const std::vector<Moses::FactorType>	&outputFactorOrder
            , const Moses::FactorMask							&inputFactorUsed
            , size_t												nBestSize
            , const std::string							&nBestFilePath
            , const std::string                                                     &inputFilePath = "");
  ~IOWrapper();

  Moses::InputType* GetInput(Moses::InputType *inputType);
  bool ReadInput(Moses::InputTypeEnum inputType, Moses::InputType*& source);

  void OutputBestHypo(const Moses::Hypothesis *hypo, long translationId, char reportSegmentation, bool reportAllFactors);
  void OutputLatticeMBRNBestList(const std::vector<LatticeMBRSolution>& solutions,long translationId);
  void Backtrack(const Moses::Hypothesis *hypo);

  Moses::OutputCollector *GetSingleBestOutputCollector() {
    return m_singleBestOutputCollector;
  }

  Moses::OutputCollector *GetNBestOutputCollector() {
    return m_nBestOutputCollector;
  }

  Moses::OutputCollector *GetUnknownsCollector() {
    return m_unknownsCollector;
  }

  Moses::OutputCollector *GetAlignmentInfoCollector() {
    return m_alignmentInfoCollector;
  }

  Moses::OutputCollector *GetSearchGraphOutputCollector() {
    return m_searchGraphOutputCollector;
  }

  Moses::OutputCollector *GetDetailedTranslationCollector() {
    return m_detailedTranslationCollector;
  }

  Moses::OutputCollector *GetWordGraphCollector() {
    return m_wordGraphCollector;
  }

  Moses::OutputCollector *GetLatticeSamplesCollector() {
    return m_latticeSamplesCollector;
  }

  // CHART
  void OutputBestHypo(const Moses::ChartHypothesis *hypo, long translationId);
  void OutputBestHypo(search::Applied applied, long translationId);
  void OutputBestHypo(const Moses::Syntax::SHyperedge *, long translationId);

  void OutputBestNone(long translationId);

  void OutputNBestList(const std::vector<boost::shared_ptr<Moses::ChartKBestExtractor::Derivation> > &nBestList, long translationId);
  void OutputNBestList(const std::vector<search::Applied> &nbest, long translationId);
  void OutputNBestList(const Moses::Syntax::KBestExtractor::KBestVec &nBestList, long translationId);

  void OutputDetailedTranslationReport(const Moses::ChartHypothesis *hypo, const Moses::Sentence &sentence, long translationId);
  void OutputDetailedTranslationReport(const search::Applied *applied, const Moses::Sentence &sentence, long translationId);
  void OutputDetailedTranslationReport(const Moses::Syntax::SHyperedge *, long translationId);

  void OutputDetailedAllTranslationReport(const std::vector<boost::shared_ptr<Moses::ChartKBestExtractor::Derivation> > &nBestList, const Moses::ChartManager &manager, const Moses::Sentence &sentence, long translationId);

  void OutputAlignment(size_t translationId , const Moses::ChartHypothesis *hypo);
  void OutputUnknowns(const std::vector<Moses::Phrase*> &, long);
  void OutputUnknowns(const std::set<Moses::Word> &, long);

  void OutputDetailedTreeFragmentsTranslationReport(const Moses::ChartHypothesis *hypo,
		  	  	  	  	  	  const Moses::Sentence &sentence,
		  	  	  	  	  	  long translationId);
  void OutputDetailedTreeFragmentsTranslationReport(const search::Applied *applied,
		  	  	  	  	  	  const Moses::Sentence &sentence,
		  	  	  	  	  	  long translationId);

  // phrase-based
  void OutputBestSurface(std::ostream &out, const Moses::Hypothesis *hypo, const std::vector<Moses::FactorType> &outputFactorOrder, char reportSegmentation, bool reportAllFactors);
  void OutputLatticeMBRNBest(std::ostream& out, const std::vector<LatticeMBRSolution>& solutions,long translationId);
  void OutputBestHypo(const std::vector<Moses::Word>&  mbrBestHypo, long /*translationId*/,
                      char reportSegmentation, bool reportAllFactors, std::ostream& out);
  void OutputBestHypo(const Moses::TrellisPath &path, long /*translationId*/,char reportSegmentation, bool reportAllFactors, std::ostream &out);
  void OutputInput(std::ostream& os, const Moses::Hypothesis* hypo);
  void OutputInput(std::vector<const Phrase*>& map, const Hypothesis* hypo);

  void OutputAlignment(Moses::OutputCollector* collector, size_t lineNo, const Moses::Hypothesis *hypo);
  void OutputAlignment(Moses::OutputCollector* collector, size_t lineNo,  const Moses::TrellisPath &path);
  void OutputAlignment(OutputCollector* collector, size_t lineNo , const std::vector<const Hypothesis *> &edges);

  static void OutputAlignment(std::ostream &out, const Moses::Hypothesis *hypo);
  static void OutputAlignment(std::ostream &out, const std::vector<const Hypothesis *> &edges);
  static void OutputAlignment(std::ostream &out, const Moses::AlignmentInfo &ai, size_t sourceOffset, size_t targetOffset);

  void OutputNBest(std::ostream& out
                   , const Moses::TrellisPathList &nBestList
                   , const std::vector<Moses::FactorType>& outputFactorOrder
                   , long translationId
                   , char reportSegmentation);

  static void OutputAllFeatureScores(const Moses::ScoreComponentCollection &features
                              , std::ostream &out);
  static void OutputFeatureScores( std::ostream& out
                            , const Moses::ScoreComponentCollection &features
                            , const Moses::FeatureFunction *ff
                            , std::string &lastName );

  // creates a map of TARGET positions which should be replaced by word using placeholder
  std::map<size_t, const Moses::Factor*> GetPlaceholders(const Moses::Hypothesis &hypo, Moses::FactorType placeholderFactor);

};



}

