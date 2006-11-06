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
#include "IOCommandLine.h"
#include "IOFile.h"
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
Timer timer;


bool readInput(IODevice *inputOutput, int inputType, InputType*& source) 
{
	delete source;
	source=inputOutput->GetInput((inputType ? 
																static_cast<InputType*>(new ConfusionNet) : 
																static_cast<InputType*>(new Sentence(Input))));
	return (source ? true : false);
}


int main(int argc, char* argv[])
{
	TRACE_ERR( "command: " );
	for(int i=0;i<argc;++i) TRACE_ERR( argv[i]<<" " );
	TRACE_ERR(endl);

	// load data structures
	timer.start();
	StaticData staticData;
	if (!staticData.LoadParameters(argc, argv))
		return EXIT_FAILURE;

	// set up read/writing class
	IODevice *inputOutput = GetInputOutput(staticData);

	// check on weights
	vector<float> weights = staticData.GetAllWeights();
	IFVERBOSE(2) {
	  std::cerr << "The score component vector looks like this:\n" << staticData.GetScoreIndexManager();
	  std::cerr << "The global weight vector looks like this:";
	  for (size_t j=0; j<weights.size(); j++) { std::cerr << " " << weights[j]; }
	  std::cerr << "\n";
	}
	// every score must have a weight!  check that here:
	if(weights.size() != staticData.GetScoreIndexManager().GetTotalNumberOfScores()) {
	  std::cerr << "ERROR: " << staticData.GetScoreIndexManager().GetTotalNumberOfScores() << " score components, but " << weights.size() << " weights defined" << std::endl;
	  return EXIT_FAILURE;
	}

	if (inputOutput == NULL)
		return EXIT_FAILURE;

	// read each sentence & decode
	InputType *source=0;
	size_t lineCount = 0;
	while(readInput(inputOutput,staticData.GetInputType(),source))
		{
			// note: source is only valid within this while loop!
    ResetUserTime();
			
    VERBOSE(2,"\nTRANSLATING(" << ++lineCount << "): " << *source);

			staticData.InitializeBeforeSentenceProcessing(*source);
			Manager manager(*source, staticData);
			manager.ProcessSentence();
			inputOutput->SetOutput(manager.GetBestHypothesis(), source->GetTranslationId(),
														 staticData.GetReportSegmentation(),
														 staticData.GetReportAllFactors()
														 );

			// n-best
			size_t nBestSize = staticData.GetNBestSize();
			if (nBestSize > 0)
				{
				  VERBOSE(2,"WRITING " << nBestSize << " TRANSLATION ALTERNATIVES TO " << staticData.GetNBestFilePath() << endl);
					LatticePathList nBestList;
					manager.CalcNBest(nBestSize, nBestList,staticData.OnlyDistinctNBest());
					inputOutput->SetNBest(nBestList, source->GetTranslationId());
					//RemoveAllInColl(nBestList);
				}

			if (staticData.IsDetailedTranslationReportingEnabled()) {
				TranslationAnalysis::PrintTranslationAnalysis(std::cerr, manager.GetBestHypothesis());
			}

			IFVERBOSE(2) { PrintUserTime(std::cerr, "Sentence Decoding Time:"); }
      
			manager.CalcDecoderStatistics(staticData);
			staticData.CleanUpAfterSentenceProcessing();      
      
		}
	
	delete inputOutput;

	timer.check("End.");
	return EXIT_SUCCESS;
}

IODevice *GetInputOutput(StaticData &staticData)
{
	IODevice *inputOutput;
	const std::vector<FactorType> &inputFactorOrder = staticData.GetInputFactorOrder()
																,&outputFactorOrder = staticData.GetOutputFactorOrder();
	FactorMask inputFactorUsed(inputFactorOrder);

	// io
	if (staticData.GetIOMethod() == IOMethodFile)
	{
	  VERBOSE(2,"IO from File" << endl);
		string					inputFileHash;
		list< Phrase >	inputPhraseList;
		string filePath = staticData.GetParam("input-file")[0];

		VERBOSE(2,"About to create ioFile" << endl);
		IOFile *ioFile = new IOFile(inputFactorOrder, outputFactorOrder, inputFactorUsed
																	, staticData.GetFactorCollection()
																	, staticData.GetNBestSize()
																	, staticData.GetNBestFilePath()
																	, filePath);
		if(staticData.GetInputType()) 
			{
				TRACE_ERR("Do not read input phrases for confusion net translation\n");
			}
		else
			{
			  VERBOSE(2,"About to GetInputPhrase\n");
				ioFile->GetInputPhrase(inputPhraseList);
			}
		VERBOSE(2,"After GetInputPhrase" << endl);
		inputOutput = ioFile;
		inputFileHash = GetMD5Hash(filePath);
		VERBOSE(2,"About to LoadPhraseTables" << endl);
		staticData.LoadPhraseTables(true, inputFileHash, inputPhraseList);
		ioFile->ResetTranslationId();
	}
	else
	{
	  VERBOSE(1,"IO from STDOUT/STDIN" << endl);
		inputOutput = new IOCommandLine(inputFactorOrder, outputFactorOrder, inputFactorUsed
																	, staticData.GetFactorCollection()
																	, staticData.GetNBestSize()
																	, staticData.GetNBestFilePath());
		staticData.LoadPhraseTables();
	}
	staticData.LoadMapping();
	timer.check("Created input-output object");

	return inputOutput;
}
