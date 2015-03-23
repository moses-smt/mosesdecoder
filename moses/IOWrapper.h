// -*- c++ -*-
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

#ifdef WITH_THREADS
#include <boost/thread.hpp>
#endif

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
class TranslationTask;
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

  std::auto_ptr<Moses::OutputCollector> m_singleBestOutputCollector;
  std::auto_ptr<Moses::OutputCollector> m_nBestOutputCollector;
  std::auto_ptr<Moses::OutputCollector> m_unknownsCollector;
  std::auto_ptr<Moses::OutputCollector> m_alignmentInfoCollector;
  std::auto_ptr<Moses::OutputCollector> m_searchGraphOutputCollector;
  std::auto_ptr<Moses::OutputCollector> m_detailedTranslationCollector;
  std::auto_ptr<Moses::OutputCollector> m_wordGraphCollector;
  std::auto_ptr<Moses::OutputCollector> m_latticeSamplesCollector;
  std::auto_ptr<Moses::OutputCollector> m_detailTreeFragmentsOutputCollector;

  bool m_surpressSingleBestOutput;

#ifdef WITH_THREADS
  boost::mutex m_lock;
#endif
  size_t m_currentLine; /* line counter, initialized from static data at construction
			 * incremented with every call to ReadInput */

  InputTypeEnum m_inputType; // initialized from StaticData at construction

public:
  IOWrapper();
  ~IOWrapper();

  // Moses::InputType* GetInput(Moses::InputType *inputType);
  boost::shared_ptr<InputType> ReadInput();

  Moses::OutputCollector *GetSingleBestOutputCollector() {
    return m_singleBestOutputCollector.get();
  }

  void SetOutputStream2SingleBestOutputCollector(std::ostream* outStream) {
    if (m_singleBestOutputCollector.get())
      m_singleBestOutputCollector->SetOutputStream(outStream);
    else
      m_singleBestOutputCollector.reset(new Moses::OutputCollector(outStream));
  }

  Moses::OutputCollector *GetNBestOutputCollector() {
    return m_nBestOutputCollector.get();
  }

  Moses::OutputCollector *GetUnknownsCollector() {
    return m_unknownsCollector.get();
  }

  Moses::OutputCollector *GetAlignmentInfoCollector() {
    return m_alignmentInfoCollector.get();
  }

  Moses::OutputCollector *GetSearchGraphOutputCollector() {
    return m_searchGraphOutputCollector.get();
  }

  Moses::OutputCollector *GetDetailedTranslationCollector() {
    return m_detailedTranslationCollector.get();
  }

  Moses::OutputCollector *GetWordGraphCollector() {
    return m_wordGraphCollector.get();
  }

  Moses::OutputCollector *GetLatticeSamplesCollector() {
    return m_latticeSamplesCollector.get();
  }

  Moses::OutputCollector *GetDetailTreeFragmentsOutputCollector() {
    return m_detailTreeFragmentsOutputCollector.get();
  }

  void SetInputStreamFromString(std::istringstream &input){
    m_inputStream = &input;
  }

  // post editing
  std::ifstream *spe_src, *spe_trg, *spe_aln;

};



}

