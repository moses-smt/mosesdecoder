// $Id: IOWrapper.h 2615 2009-11-12 13:18:20Z hieuhoang1972 $

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
#include "TypeDef.h"
#include "Sentence.h"
#include "FactorTypeSet.h"
#include "TrellisPathList.h"
#include "../../moses-chart/src/ChartHypothesis.h"

namespace Moses
{
class FactorCollection;
}

namespace MosesChart
{
class TrellisPathList;
}

class IOWrapper
{
protected:
	long m_translationId;

	const std::vector<Moses::FactorType>	&m_inputFactorOrder;
	const std::vector<Moses::FactorType>	&m_outputFactorOrder;
	const Moses::FactorMask								&m_inputFactorUsed;
	std::ostream 									*m_nBestStream, *m_outputSearchGraphStream;
	std::string										m_inputFilePath;
	std::istream									*m_inputStream;
	bool													m_surpressSingleBestOutput;
	
public:
	IOWrapper(const std::vector<Moses::FactorType>	&inputFactorOrder
				, const std::vector<Moses::FactorType>	&outputFactorOrder
				, const Moses::FactorMask							&inputFactorUsed
				, size_t												nBestSize
				, const std::string							&nBestFilePath
				, const std::string							&inputFilePath="");
	~IOWrapper();

	Moses::InputType* GetInput(Moses::InputType *inputType);
	void OutputBestHypo(const MosesChart::Hypothesis *hypo, long translationId, bool reportSegmentation, bool reportAllFactors);
	void OutputBestHypo(const std::vector<const Moses::Factor*>&  mbrBestHypo, long translationId, bool reportSegmentation, bool reportAllFactors);
	void OutputNBestList(const MosesChart::TrellisPathList &nBestList, long translationId);
	void Backtrack(const MosesChart::Hypothesis *hypo);

	void ResetTranslationId() { m_translationId = 0; }

	std::ostream &GetOutputSearchGraphStream()
	{ return *m_outputSearchGraphStream; }
	
};
