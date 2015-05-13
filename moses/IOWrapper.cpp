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

#include <iostream>
#include <stack>
#include <boost/algorithm/string.hpp>

#include "moses/Syntax/KBestExtractor.h"
#include "moses/Syntax/PVertex.h"
#include "moses/Syntax/SHyperedge.h"
#include "moses/Syntax/S2T/DerivationWriter.h"
#include "moses/Syntax/SVertex.h"

#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/Hypothesis.h"
#include "moses/WordsRange.h"
#include "moses/TrellisPathList.h"
#include "moses/StaticData.h"
#include "moses/FeatureVector.h"
#include "moses/InputFileStream.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/TreeInput.h"
#include "moses/ForestInput.h"
#include "moses/ConfusionNet.h"
#include "moses/WordLattice.h"
#include "moses/Incremental.h"
#include "moses/ChartManager.h"


#include "util/exception.hh"

#include "IOWrapper.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

using namespace std;

namespace Moses
{

IOWrapper::IOWrapper()
  : m_nBestStream(NULL)
  , m_outputWordGraphStream(NULL)
  , m_outputSearchGraphStream(NULL)
  , m_detailedTranslationReportingStream(NULL)
  , m_unknownsStream(NULL)
  , m_alignmentInfoStream(NULL)
  , m_latticeSamplesStream(NULL)
  , m_surpressSingleBestOutput(false)
  , m_look_ahead(0)
  , m_look_back(0)
  , m_buffered_ahead(0)
  , spe_src(NULL)
  , spe_trg(NULL)
  , spe_aln(NULL)
{
  const StaticData &staticData = StaticData::Instance();

  // context buffering for context-sensitive decoding
  m_look_ahead = staticData.GetContextParameters().look_ahead;
  m_look_back  = staticData.GetContextParameters().look_back;

  m_inputType = staticData.GetInputType();

  UTIL_THROW_IF2((m_look_ahead || m_look_back) && m_inputType != SentenceInput,
                 "Context-sensitive decoding currently works only with sentence input.");

  m_currentLine = staticData.GetStartTranslationId();

  m_inputFactorOrder = &staticData.GetInputFactorOrder();

  size_t nBestSize = staticData.GetNBestSize();
  string nBestFilePath = staticData.GetNBestFilePath();

  staticData.GetParameter().SetParameter<string>(m_inputFilePath, "input-file", "");
  if (m_inputFilePath.empty()) {
    m_inputFile = NULL;
    m_inputStream = &cin;
  } else {
    VERBOSE(2,"IO from File" << endl);
    m_inputFile = new InputFileStream(m_inputFilePath);
    m_inputStream = m_inputFile;
  }

  if (nBestSize > 0) {
    if (nBestFilePath == "-" || nBestFilePath == "/dev/stdout") {
      m_nBestStream = &std::cout;
      m_nBestOutputCollector.reset(new Moses::OutputCollector(&std::cout));
      m_surpressSingleBestOutput = true;
    } else {
      std::ofstream *file = new std::ofstream;
      file->open(nBestFilePath.c_str());
      m_nBestStream = file;

      m_nBestOutputCollector.reset(new Moses::OutputCollector(file));
      //m_nBestOutputCollector->HoldOutputStream();
    }
  }

  // search graph output
  if (staticData.GetOutputSearchGraph()) {
    string fileName;
    if (staticData.GetOutputSearchGraphExtended()) {
      staticData.GetParameter().SetParameter<string>(fileName, "output-search-graph-extended", "");
    } else {
      staticData.GetParameter().SetParameter<string>(fileName, "output-search-graph", "");
    }
    std::ofstream *file = new std::ofstream;
    m_outputSearchGraphStream = file;
    file->open(fileName.c_str());
  }

  if (!staticData.GetOutputUnknownsFile().empty()) {
    m_unknownsStream = new std::ofstream(staticData.GetOutputUnknownsFile().c_str());
    m_unknownsCollector.reset(new Moses::OutputCollector(m_unknownsStream));
    UTIL_THROW_IF2(!m_unknownsStream->good(),
                   "File for unknowns words could not be opened: " <<
                   staticData.GetOutputUnknownsFile());
  }

  if (!staticData.GetAlignmentOutputFile().empty()) {
    m_alignmentInfoStream = new std::ofstream(staticData.GetAlignmentOutputFile().c_str());
    m_alignmentInfoCollector.reset(new Moses::OutputCollector(m_alignmentInfoStream));
    UTIL_THROW_IF2(!m_alignmentInfoStream->good(),
                   "File for alignment output could not be opened: " << staticData.GetAlignmentOutputFile());
  }

  if (staticData.GetOutputSearchGraph()) {
    string fileName;
    staticData.GetParameter().SetParameter<string>(fileName, "output-search-graph", "");

    std::ofstream *file = new std::ofstream;
    m_outputSearchGraphStream = file;
    file->open(fileName.c_str());
    m_searchGraphOutputCollector.reset(new Moses::OutputCollector(m_outputSearchGraphStream));
  }

  // detailed translation reporting
  if (staticData.IsDetailedTranslationReportingEnabled()) {
    const std::string &path = staticData.GetDetailedTranslationReportingFilePath();
    m_detailedTranslationReportingStream = new std::ofstream(path.c_str());
    m_detailedTranslationCollector.reset(new Moses::OutputCollector(m_detailedTranslationReportingStream));
  }

  if (staticData.IsDetailedTreeFragmentsTranslationReportingEnabled()) {
    const std::string &path = staticData.GetDetailedTreeFragmentsTranslationReportingFilePath();
    m_detailedTreeFragmentsTranslationReportingStream = new std::ofstream(path.c_str());
    m_detailTreeFragmentsOutputCollector.reset(new Moses::OutputCollector(m_detailedTreeFragmentsTranslationReportingStream));
  }

  // wordgraph output
  if (staticData.GetOutputWordGraph()) {
    string fileName;
    staticData.GetParameter().SetParameter<string>(fileName, "output-word-graph", "");

    std::ofstream *file = new std::ofstream;
    m_outputWordGraphStream  = file;
    file->open(fileName.c_str());
    m_wordGraphCollector.reset(new OutputCollector(m_outputWordGraphStream));
  }

  size_t latticeSamplesSize = staticData.GetLatticeSamplesSize();
  string latticeSamplesFile = staticData.GetLatticeSamplesFilePath();
  if (latticeSamplesSize) {
    if (latticeSamplesFile == "-" || latticeSamplesFile == "/dev/stdout") {
      m_latticeSamplesCollector.reset(new OutputCollector());
      m_surpressSingleBestOutput = true;
    } else {
      m_latticeSamplesStream = new ofstream(latticeSamplesFile.c_str());
      if (!m_latticeSamplesStream->good()) {
        TRACE_ERR("ERROR: Failed to open " << latticeSamplesFile << " for lattice samples" << endl);
        exit(1);
      }
      m_latticeSamplesCollector.reset(new OutputCollector(m_latticeSamplesStream));
    }
  }

  if (!m_surpressSingleBestOutput) {
    m_singleBestOutputCollector.reset(new Moses::OutputCollector(&std::cout));
  }

  // setup file pattern for hypergraph output
  char const* key = "output-search-graph-hypergraph";
  PARAM_VEC const* p = staticData.GetParameter().GetParam(key);
  std::string& fmt = m_hypergraph_output_filepattern;
  // first, determine the output directory
  if (p && p->size() > 2) fmt = p->at(2);
  else if (nBestFilePath.size() && nBestFilePath != "-" &&
           ! boost::starts_with(nBestFilePath, "/dev/stdout")) {
    fmt = boost::filesystem::path(nBestFilePath).parent_path().string();
    if (fmt.empty()) fmt = ".";
  } else fmt = boost::filesystem::current_path().string() + "/hypergraph";
  if (*fmt.rbegin() != '/') fmt += "/";
  std::string extension = (p && p->size() > 1 ? p->at(1) : std::string("txt"));
  UTIL_THROW_IF2(extension != "txt" && extension != "gz" && extension != "bz2",
                 "Unknown compression type '" << extension
                 << "' for hypergraph output!");
  fmt += string("%d.") + extension;

  if (staticData.GetParameter().GetParam("spe-src")) {
    spe_src = new ifstream(staticData.GetParameter().GetParam("spe-src")->at(0).c_str());
    spe_trg = new ifstream(staticData.GetParameter().GetParam("spe-trg")->at(0).c_str());
    spe_aln = new ifstream(staticData.GetParameter().GetParam("spe-aln")->at(0).c_str());
  }
}

IOWrapper::~IOWrapper()
{
  if (m_inputFile != NULL)
    delete m_inputFile;
  if (m_nBestStream != NULL && !m_surpressSingleBestOutput) {
    // outputting n-best to file, rather than stdout. need to close file and delete obj
    delete m_nBestStream;
  }

  delete m_detailedTranslationReportingStream;
  delete m_alignmentInfoStream;
  delete m_unknownsStream;
  delete m_outputSearchGraphStream;
  delete m_outputWordGraphStream;
  delete m_latticeSamplesStream;
}

// InputType*
// IOWrapper::
// GetInput(InputType* inputType)
// {
//   if(inputType->Read(*m_inputStream, *m_inputFactorOrder)) {
//     return inputType;
//   } else {
//     delete inputType;
//     return NULL;
//   }
// }

boost::shared_ptr<InputType>
IOWrapper::
GetBufferedInput()
{
  switch(m_inputType) {
  case SentenceInput:
    return BufferInput<Sentence>();
  case ConfusionNetworkInput:
    return BufferInput<ConfusionNet>();
  case WordLatticeInput:
    return BufferInput<WordLattice>();
  case TreeInputType:
    return BufferInput<TreeInput>();
  case TabbedSentenceInput:
    return BufferInput<TabbedSentence>();
  case ForestInputType:
    return BufferInput<ForestInput>();
  default:
    TRACE_ERR("Unknown input type: " << m_inputType << "\n");
    return boost::shared_ptr<InputType>();
  }

}

boost::shared_ptr<InputType>
IOWrapper::ReadInput()
{
#ifdef WITH_THREADS
  boost::lock_guard<boost::mutex> lock(m_lock);
#endif
  boost::shared_ptr<InputType> source = GetBufferedInput();
  if (source) {
    source->SetTranslationId(m_currentLine++);
    this->set_context_for(*source);
  }
  m_past_input.push_back(source);
  return source;
}

void
IOWrapper::
set_context_for(InputType& source)
{
  boost::shared_ptr<string> context(new string);
  list<boost::shared_ptr<InputType> >::iterator m = m_past_input.end();
  // remove obsolete past input from buffer:
  if (m_past_input.end() != m_past_input.begin()) {
    for (size_t cnt = 0; cnt < m_look_back && --m != m_past_input.begin();
         cnt += (*m)->GetSize());
    while (m_past_input.begin() != m) m_past_input.pop_front();
  }
  // cerr << string(80,'=') << endl;
  if (m_past_input.size()) {
    m = m_past_input.begin();
    *context += (*m)->ToString();
    // cerr << (*m)->ToString() << endl;
    for (++m; m != m_past_input.end(); ++m) {
      // cerr << "\n" << (*m)->ToString() << endl;
      *context += string(" ") + (*m)->ToString();
    }
    // cerr << string(80,'-') << endl;
  }
  // cerr << source.ToString() << endl;
  if (m_future_input.size()) {
    // cerr << string(80,'-') << endl;
    for (m = m_future_input.begin(); m != m_future_input.end(); ++m) {
      // if (m != m_future_input.begin()) cerr << "\n";
      // cerr << (*m)->ToString() << endl;
      if (context->size()) *context += " ";
      *context += (*m)->ToString();
    }
  }
  // cerr << string(80,'=') << endl;
  source.SetContext(context);
}



std::string
IOWrapper::
GetHypergraphOutputFileName(size_t const id) const
{
  return str(boost::format(m_hypergraph_output_filepattern) % id);
}


} // namespace

