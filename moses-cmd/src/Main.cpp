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

#ifdef WIN32
// Include Visual Leak Detector
#include <vld.h>
#endif

#include <fstream>
#include "Main.h"
#include "LatticePath.h"
#include "FactorCollection.h"
#include "Manager.h"
#include "Phrase.h"
#include "Util.h"
#include "LatticePathList.h"
#include "Timer.h"
#include "IOStream.h"
#include "Sentence.h"
#include "ConfusionNet.h"
#include "TranslationAnalysis.h"

#if HAVE_CONFIG_H
#include "config.h"
#else
// those not using autoconf have to build MySQL support for now
#  define USE_MYSQL 1
#endif

using namespace std;

bool readInput(IOStream &ioStream, int inputType, InputType*& source) 
{
	delete source;
	source=ioStream.GetInput((inputType ? 
																static_cast<InputType*>(new ConfusionNet) : 
																static_cast<InputType*>(new Sentence(Input))));
	return (source ? true : false);
}


int main(int argc, char* argv[])
{
	TRACE_ERR("command: ");
	for(int i=0;i<argc;++i) TRACE_ERR(argv[i]<<" ");
	TRACE_ERR(endl);

	// load data structures
	Parameter *parameter = new Parameter();
	if (!parameter->LoadParam(argc, argv))
	{
		parameter->Explain();
		delete parameter;
		return EXIT_FAILURE;		
	}

	const StaticData &staticData = StaticData::Instance();
	if (!StaticData::LoadDataStatic(parameter))
		return EXIT_FAILURE;

	// set up read/writing class
	IOStream *ioStream = GetIODevice(staticData);

	// check on weights
	vector<float> weights = staticData.GetAllWeights();
	IFVERBOSE(2) {
	  TRACE_ERR("The score component vector looks like this:\n" << staticData.GetScoreIndexManager());
	  TRACE_ERR("The global weight vector looks like this:");
	  for (size_t j=0; j<weights.size(); j++) { TRACE_ERR(" " << weights[j]); }
	  TRACE_ERR("\n");
	}
	// every score must have a weight!  check that here:
	if(weights.size() != staticData.GetScoreIndexManager().GetTotalNumberOfScores()) {
	  TRACE_ERR("ERROR: " << staticData.GetScoreIndexManager().GetTotalNumberOfScores() << " score components, but " << weights.size() << " weights defined" << std::endl);
	  return EXIT_FAILURE;
	}

	if (ioStream == NULL)
		return EXIT_FAILURE;

	// read each sentence & decode
	InputType *source=0;
	size_t lineCount = 0;
	while(readInput(*ioStream,staticData.GetInputType(),source))
	{
			// note: source is only valid within this while loop!
    ResetUserTime();
			
    VERBOSE(2,"\nTRANSLATING(" << ++lineCount << "): " << *source);

		staticData.InitializeBeforeSentenceProcessing(*source);
		Manager manager(*source);
		manager.ProcessSentence();
		ioStream->OutputBestHypo(manager.GetBestHypothesis(), source->GetTranslationId(),
													 staticData.GetReportSegmentation(),
													 staticData.GetReportAllFactors()
													 );
		IFVERBOSE(2) { PrintUserTime("Best Hypothesis Generation Time:"); }

		// n-best
		size_t nBestSize = staticData.GetNBestSize();
		if (nBestSize > 0)
			{
			  VERBOSE(2,"WRITING " << nBestSize << " TRANSLATION ALTERNATIVES TO " << staticData.GetNBestFilePath() << endl);
				LatticePathList nBestList;
				manager.CalcNBest(nBestSize, nBestList,staticData.GetDistinctNBest());
				ioStream->OutputNBestList(nBestList, source->GetTranslationId());
				//RemoveAllInColl(nBestList);

				IFVERBOSE(2) { PrintUserTime("N-Best Hypotheses Generation Time:"); }
		}

		if (staticData.IsDetailedTranslationReportingEnabled()) {
			TranslationAnalysis::PrintTranslationAnalysis(std::cerr, manager.GetBestHypothesis());
		}

		IFVERBOSE(2) { PrintUserTime("Sentence Decoding Time:"); }
    
		manager.CalcDecoderStatistics(staticData);
		staticData.CleanUpAfterSentenceProcessing();      
    
	}
	
	delete ioStream;

	PrintUserTime("End.");

	#ifdef HACK_EXIT
	//This avoids that detructors are called (it can take a long time)
		exit(EXIT_SUCCESS);
	#else
		return EXIT_SUCCESS;
	#endif
}

IOStream *GetIODevice(const StaticData &staticData)
{
	IOStream *ioStream;
	const std::vector<FactorType> &inputFactorOrder = staticData.GetInputFactorOrder()
																,&outputFactorOrder = staticData.GetOutputFactorOrder();
	FactorMask inputFactorUsed(inputFactorOrder);

	// io
	if (staticData.GetParam("input-file").size() == 1)
	{
	  VERBOSE(2,"IO from File" << endl);
		string filePath = staticData.GetParam("input-file")[0];

		ioStream = new IOStream(inputFactorOrder, outputFactorOrder, inputFactorUsed
																	, staticData.GetNBestSize()
																	, staticData.GetNBestFilePath()
																	, filePath);
	}
	else
	{
	  VERBOSE(1,"IO from STDOUT/STDIN" << endl);
		ioStream = new IOStream(inputFactorOrder, outputFactorOrder, inputFactorUsed
																	, staticData.GetNBestSize()
																	, staticData.GetNBestFilePath());
	}
	ioStream->ResetTranslationId();

	PrintUserTime("Created input-output object");

	return ioStream;
}
