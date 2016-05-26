// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
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
#include <list>
#include <iomanip>
#include <limits>

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
#include "moses/parameters/AllOptions.h"

#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>

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
  boost::shared_ptr<AllOptions const> m_options;
  const std::vector<Moses::FactorType>	*m_inputFactorOrder;
  std::string m_inputFilePath;
  Moses::InputFileStream *m_inputFile;
  std::istream *m_inputStream;
  std::ostream *m_nBestStream;
  // std::ostream *m_outputWordGraphStream;
  // std::auto_ptr<std::ostream> m_outputSearchGraphStream;
  // std::ostream *m_detailedTranslationReportingStream;
  std::ostream *m_unknownsStream;
  // std::ostream *m_detailedTreeFragmentsTranslationReportingStream;
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
  std::list<boost::shared_ptr<InputType> > m_past_input;
  std::list<boost::shared_ptr<InputType> > m_future_input;
  size_t m_look_ahead; /// for context-sensitive decoding: # of wrds to look ahead
  size_t m_look_back;  /// for context-sensitive decoding: # of wrds to look back
  size_t m_buffered_ahead; /// number of words buffered ahead
  // For context-sensitive decoding:
  // Number of context words ahead and before the current sentence.

  std::string m_hypergraph_output_filepattern;

public:
  IOWrapper(AllOptions const& opts);
  ~IOWrapper();

  // Moses::InputType* GetInput(Moses::InputType *inputType);

  boost::shared_ptr<InputType>
  ReadInput(boost::shared_ptr<std::vector<std::string> >* cw = NULL);

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

  void SetInputStreamFromString(std::istringstream &input) {
    m_inputStream = &input;
  }

  std::string GetHypergraphOutputFileName(size_t const id) const;

  // post editing
  std::ifstream *spe_src, *spe_trg, *spe_aln;

  std::list<boost::shared_ptr<InputType> > const& GetPastInput() const {
    return m_past_input;
  }

  std::list<boost::shared_ptr<InputType> > const& GetFutureInput() const {
    return m_future_input;
  }
  size_t GetLookAhead() const {
    return m_look_ahead;
  }

  size_t GetLookBack() const {
    return m_look_back;
  }

private:
  template<class itype>
  boost::shared_ptr<InputType>
  BufferInput();

  boost::shared_ptr<InputType>
  GetBufferedInput();

  boost::shared_ptr<std::vector<std::string> >
  GetCurrentContextWindow() const;
};

template<class itype>
boost::shared_ptr<InputType>
IOWrapper::
BufferInput()
{
  boost::shared_ptr<itype>  source;
  boost::shared_ptr<InputType> ret;
  if (m_future_input.size()) {
    ret = m_future_input.front();
    m_future_input.pop_front();
    m_buffered_ahead -= ret->GetSize();
  } else {
    source.reset(new itype(m_options));
    if (!source->Read(*m_inputStream))
      return ret;
    ret = source;
  }
  while (m_buffered_ahead < m_look_ahead) {
    source.reset(new itype(m_options));
    if (!source->Read(*m_inputStream))
      break;
    m_future_input.push_back(source);
    m_buffered_ahead += source->GetSize();
  }
  return ret;
}


}

