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
#include "moses/TabbedSentence.h"
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

  const std::vector<Moses::FactorType>	*m_inputFactorOrder;
  std::string m_inputFilePath;
  Moses::InputFileStream *m_inputFile;
  std::istream *m_inputStream;
  std::ostream *m_nBestStream;
  std::ostream *m_outputWordGraphStream;
  std::ostream *m_outputSearchGraphStream;
  std::ostream *m_detailedTranslationReportingStream;
  std::ostream *m_unknownsStream;
  std::ostream *m_detailedTreeFragmentsTranslationReportingStream;
  std::ofstream *m_alignmentInfoStream;
  std::ofstream *m_latticeSamplesStream;

  Moses::OutputCollector *m_singleBestOutputCollector;
  Moses::OutputCollector *m_nBestOutputCollector;
  Moses::OutputCollector *m_unknownsCollector;
  Moses::OutputCollector *m_alignmentInfoCollector;
  Moses::OutputCollector *m_searchGraphOutputCollector;
  Moses::OutputCollector *m_detailedTranslationCollector;
  Moses::OutputCollector *m_wordGraphCollector;
  Moses::OutputCollector *m_latticeSamplesCollector;
  Moses::OutputCollector *m_detailTreeFragmentsOutputCollector;

  bool m_surpressSingleBestOutput;


public:
  IOWrapper();
  ~IOWrapper();

  Moses::InputType* GetInput(Moses::InputType *inputType);
  bool ReadInput(Moses::InputTypeEnum inputType, Moses::InputType*& source);

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

  Moses::OutputCollector *GetDetailTreeFragmentsOutputCollector() {
    return m_detailTreeFragmentsOutputCollector;
  }

  // post editing
  std::ifstream *spe_src, *spe_trg, *spe_aln;

};



}

