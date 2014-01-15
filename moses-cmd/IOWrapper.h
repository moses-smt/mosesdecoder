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

#ifndef moses_cmd_IOWrapper_h
#define moses_cmd_IOWrapper_h

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
#include "LatticeMBR.h"

namespace Moses
{
class ScoreComponentCollection;
class Hypothesis;
class Factor;
}

namespace MosesCmd
{

/** Helper class that holds misc variables to write data out to command line.
 */
class IOWrapper
{
protected:
  long m_translationId;

  const std::vector<Moses::FactorType>	&m_inputFactorOrder;
  const std::vector<Moses::FactorType>	&m_outputFactorOrder;
  const Moses::FactorMask							&m_inputFactorUsed;
  std::string										m_inputFilePath;
  Moses::InputFileStream				*m_inputFile;
  std::istream									*m_inputStream;
  std::ostream 									*m_nBestStream
  ,*m_outputWordGraphStream,*m_outputSearchGraphStream;
  std::ostream                  *m_detailedTranslationReportingStream;
  std::ofstream *m_alignmentOutputStream;
  bool													m_surpressSingleBestOutput;

  void Initialization(const std::vector<Moses::FactorType>	&inputFactorOrder
                      , const std::vector<Moses::FactorType>			&outputFactorOrder
                      , const Moses::FactorMask							&inputFactorUsed
                      , size_t												nBestSize
                      , const std::string							&nBestFilePath);


public:
  IOWrapper(const std::vector<Moses::FactorType>	&inputFactorOrder
            , const std::vector<Moses::FactorType>			&outputFactorOrder
            , const Moses::FactorMask							&inputFactorUsed
            , size_t												nBestSize
            , const std::string							&nBestFilePath);

  IOWrapper(const std::vector<Moses::FactorType>	&inputFactorOrder
            , const std::vector<Moses::FactorType>	&outputFactorOrder
            , const Moses::FactorMask							&inputFactorUsed
            , size_t												nBestSize
            , const std::string							&nBestFilePath
            , const std::string                                                     &infilePath);
  ~IOWrapper();

  Moses::InputType* GetInput(Moses::InputType *inputType);

  void OutputBestHypo(const Moses::Hypothesis *hypo, long translationId, char reportSegmentation, bool reportAllFactors);
  void OutputLatticeMBRNBestList(const std::vector<LatticeMBRSolution>& solutions,long translationId);
  void Backtrack(const Moses::Hypothesis *hypo);

  void ResetTranslationId() {
    m_translationId = 0;
  }

  std::ofstream *GetAlignmentOutputStream() {
    return m_alignmentOutputStream;
  }

  std::ostream &GetOutputWordGraphStream() {
    return *m_outputWordGraphStream;
  }
  std::ostream &GetOutputSearchGraphStream() {
    return *m_outputSearchGraphStream;
  }

  std::ostream &GetDetailedTranslationReportingStream() {
    assert (m_detailedTranslationReportingStream);
    return *m_detailedTranslationReportingStream;
  }
};

IOWrapper *GetIOWrapper(const Moses::StaticData &staticData);
bool ReadInput(IOWrapper &ioWrapper, Moses::InputTypeEnum inputType, Moses::InputType*& source);
void OutputLanguageModelOrder(std::ostream &out, const Moses::Hypothesis *hypo, Moses::Manager &manager);
void OutputBestSurface(std::ostream &out, const Moses::Hypothesis *hypo, const std::vector<Moses::FactorType> &outputFactorOrder, char reportSegmentation, bool reportAllFactors);
void OutputLatticeMBRNBest(std::ostream& out, const std::vector<LatticeMBRSolution>& solutions,long translationId);
void OutputBestHypo(const std::vector<Moses::Word>&  mbrBestHypo, long /*translationId*/,
                    char reportSegmentation, bool reportAllFactors, std::ostream& out);
void OutputBestHypo(const Moses::TrellisPath &path, long /*translationId*/,char reportSegmentation, bool reportAllFactors, std::ostream &out);
void OutputInput(std::ostream& os, const Moses::Hypothesis* hypo);
void OutputAlignment(Moses::OutputCollector* collector, size_t lineNo, const Moses::Hypothesis *hypo);
void OutputAlignment(Moses::OutputCollector* collector, size_t lineNo,  const Moses::TrellisPath &path);
void OutputAlignment(std::ostream &out, const Moses::Hypothesis *hypo);
void OutputAlignment(std::ostream &out, const Moses::AlignmentInfo &ai, size_t sourceOffset, size_t targetOffset);

void OutputNBest(std::ostream& out
                 , const Moses::TrellisPathList &nBestList
                 , const std::vector<Moses::FactorType>& outputFactorOrder
                 , long translationId
                 , char reportSegmentation);
void OutputAllFeatureScores(const Moses::ScoreComponentCollection &features
                            , std::ostream &out);
void OutputFeatureScores( std::ostream& out
                          , const Moses::ScoreComponentCollection &features
                          , const Moses::FeatureFunction *ff
                          , std::string &lastName );

// creates a map of TARGET positions which should be replaced by word using placeholder
std::map<size_t, const Moses::Factor*> GetPlaceholders(const Moses::Hypothesis &hypo, Moses::FactorType placeholderFactor);

}

#endif
