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
//#include "UnknownWordHandler.h"

#include "Sentence.h"
#include "ConfusionNet.h"

#if HAVE_CONFIG_H
#include "config.h"
#  ifdef HAVE_MYSQLPP
#    define USE_MYSQL 1
#  endif
#else
// those not using autoconf have to build MySQL support for now
#  define USE_MYSQL 1
#endif

#undef USE_MYSQL
#ifdef USE_MYSQL
#include "IOMySQL.h"
#endif

using namespace std;
Timer timer;

int main(int argc, char* argv[])
{
	timer.start("Starting...");

	StaticData staticData;
	if (!staticData.LoadParameters(argc, argv))
		return EXIT_FAILURE;
/*
	boost::shared_ptr<UnknownWordHandler> unknownWordHandler(new UnknownWordHandler);
	staticData.SetUnknownWordHandler(unknownWordHandler);
*/
		if (staticData.GetVerboseLevel() > 0)
		{
#if N_BEST
		std::cerr << "N_BEST=enabled\n";
#else
		std::cerr << "N_BEST=disabled\n";
#endif
		}

	// set up read/writing class
	InputOutput *inputOutput = GetInputOutput(staticData);

	if (inputOutput == NULL)
		return EXIT_FAILURE;

	// read each sentence & decode
	while(InputType *source = inputOutput->GetInput((staticData.GetInputType() ? static_cast<InputType*>(new ConfusionNet) : static_cast<InputType*>(new Sentence(Input)))))
	{
		if(Sentence* sent=dynamic_cast<Sentence*>(source)) 
			{TRACE_ERR(*sent<<"\n");}
		else if(ConfusionNet *cn=dynamic_cast<ConfusionNet*>(source)) 
			{cn->Print(std::cerr);std::cerr<<"\n";}

		TranslationOptionCollection *translationOptionCollection=source->CreateTranslationOptionCollection();
		assert(translationOptionCollection);

		staticData.InitializeBeforeSentenceProcessing(*source);
		Manager manager(*source, *translationOptionCollection, staticData);
		manager.ProcessSentence();
		inputOutput->SetOutput(manager.GetBestHypothesis(), source->GetTranslationId());

		// n-best
		size_t nBestSize = staticData.GetNBestSize();
		if (nBestSize > 0)
		{
			TRACE_ERR(nBestSize << " " << staticData.GetNBestFilePath() << endl);
			LatticePathList nBestList;
			manager.CalcNBest(nBestSize, nBestList);
			inputOutput->SetNBest(nBestList, source->GetTranslationId());
			RemoveAllInColl< LatticePathList::iterator > (nBestList);
		}

		// delete source
		//		inputOutput->Release(source);
		staticData.CleanUpAfterSentenceProcessing();
		delete source;
		delete translationOptionCollection;
	}
	
	delete inputOutput;


	timer.check("End.");
	return EXIT_SUCCESS;
}

InputOutput *GetInputOutput(StaticData &staticData)
{
	InputOutput *inputOutput;
	const std::vector<FactorType> &factorOrder = staticData.GetInputFactorOrder();
	FactorTypeSet inputFactorUsed(factorOrder);

	// io
	if (staticData.GetIOMethod() == IOMethodMySQL)
	{
		TRACE_ERR("IO from MySQL" << endl);
#if USE_MYSQL
		const PARAM_VEC &mySQLParam = staticData.GetParam("mysql");
		inputOutput = new IOMySQL(mySQLParam[0], mySQLParam[1], mySQLParam[2], mySQLParam[3]
														, Scan<long>(mySQLParam[4]), Scan<long>(mySQLParam[5])
														, factorOrder, inputFactorUsed, staticData.GetFactorCollection());
		staticData.LoadPhraseTables();
#else
		TRACE_ERR( "moses was not built with mysql libraries, please configure\n"
							<< "  to use another input method.\n");
		inputOutput = NULL;
#endif
	}
	else if (staticData.GetIOMethod() == IOMethodFile)
	{
		TRACE_ERR("IO from File" << endl);
		string					inputFileHash;
		list< Phrase >	inputPhraseList;
		string filePath = staticData.GetParam("input-file")[0];

		TRACE_ERR("About to create ioFile" << endl);
		IOFile *ioFile = new IOFile(factorOrder, inputFactorUsed
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
				TRACE_ERR("About to GetInputPhrase\n");
				ioFile->GetInputPhrase(inputPhraseList);
			}
		TRACE_ERR("After GetInputPhrase" << endl);
		inputOutput = ioFile;
		inputFileHash = GetMD5Hash(filePath);
		TRACE_ERR("About to LoadPhraseTables" << endl);
		staticData.LoadPhraseTables(true, inputFileHash, inputPhraseList);
	}
	else
	{
		TRACE_ERR("IO from STDOUT/STDIN" << endl);
		inputOutput = new IOCommandLine(factorOrder, inputFactorUsed
																	, staticData.GetFactorCollection()
																	, staticData.GetNBestSize()
																	, staticData.GetNBestFilePath());
		staticData.LoadPhraseTables();
	}
	staticData.LoadMapping();
	timer.check("Created input-output object");

	return inputOutput;
}

