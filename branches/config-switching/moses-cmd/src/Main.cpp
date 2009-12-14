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
#include <iostream> 
#include <sstream> 
#include <string> 
#include "Main.h"
#include "TrellisPath.h"
#include "FactorCollection.h"
#include "Manager.h"
#include "Phrase.h"
#include "Util.h"
#include "TrellisPathList.h"
#include "Timer.h"
#include "IOWrapper.h"
#include "Sentence.h"
#include "ConfusionNet.h"
#include "WordLattice.h"
#include "TranslationAnalysis.h"
#include "mbr.h"
#include "TypeDef.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_PROTOBUF
#include "hypergraph.pb.h"
#endif


using namespace std;
using namespace Moses;


InputTag CheckTag(const InputType *line, string** cmd, int *id)
{
	int id_temp;
	stringstream stream;
	string str, id_str;
	stream << *line;
	str = stream.str();

	if ( str.find("<decoder ") == 0 )
	{
	  // remove last char due to the stream
	  str = str.substr(0,(str.size()-1));
	  *cmd = new string(str.c_str());
	  VERBOSE(1, "detected a command: " << **cmd << endl);
	  return Command;
	} 
	else if ( str.find("<use config ") == 0 )
	{
	  size_t pos = str.find(">");
	  if (pos == string::npos)
	  {
	    VERBOSE(1, "detected config id tag, but error format." << endl);
	    return InvalidTag;  // ignore the tag
	  }
	  id_str = str.substr(12,(pos-12));
	  id_temp = Scan<int>(id_str);
	  // check if this id is valid......
	  if (id_temp<2 && id_temp>=0) 
	  {
	    *id = id_temp;
	    VERBOSE(1, "detected the config id: " << id_str << " = "<<*id << endl);
	    return ConfigId;
	  }
	  else
	  {
	    VERBOSE(1, "detected config id tag, but id exceeds the range." << endl);
	    return InvalidTag;  // ignore the tag
	  }
	} 
	else
	{
	  return Source;
	}
}

bool CommandIsQuit(string* cmd) 
{
	return cmd->find("<decoder quit>") != string::npos;
}

void ParseCommand(string* cmd, int* argc, vector<string>* argv)
{
	*argc=0;
	size_t len = 0;
	cmd->erase(cmd->find_last_not_of(" ")+1);// strip

	// <decoder config="...">
	size_t posStart = cmd->find("<decoder ") + 9;
	size_t posEnd = cmd->find("=");

	if ((len=(posEnd-posStart))<=0) { return; }
	string commandType(cmd->substr(posStart, len));
	if ((len=(cmd->size()-2-posEnd-2))<=0) { return; }
	string commandArgs(cmd->substr(posEnd+2, len));

	istringstream stream( commandArgs ) ;
	int args_len = commandArgs.size();
	int read_len = 0;
	string item ;

	(*argc)++;
	argv->push_back(commandType);

	while ( read_len < args_len){
	  stream >> item;
	  (*argc)++;
	  read_len += (item.size()+1);
	  argv->push_back(item);
	}

}

int Reconfig(int argc, char* argv[])
{
	// load data structures
	Parameter *parameter = new Parameter();
	if (!parameter->LoadParam(argc, argv))
	{
		parameter->Explain();
		delete parameter;
		TRACE_ERR("parameters wrong! \n");
		return EXIT_SUCCESS;
	}

	const StaticData &staticData = StaticData::Instance();

	// precheck and prepare static data
	if (!StaticData::SetUpBeforeReconfigStatic(parameter))
	{
		  TRACE_ERR("configurations wrong! \n");
		  return EXIT_SUCCESS;
	}

	// if runs here, re-configuration starts
	if (!StaticData::LoadDataStatic(parameter))
		return EXIT_FAILURE;

	// check on weights
	vector<float> weights = staticData.GetAllWeights();
	IFVERBOSE(2) {
	  TRACE_ERR("The score component vector looks like this:\n" << staticData.GetScoreIndexManager(0));
	  TRACE_ERR("The global weight vector looks like this:");
	  for (size_t j=0; j<weights.size(); j++) { TRACE_ERR(" " << weights[j]); }
	  TRACE_ERR("\n");
	}
	// every score must have a weight!  check that here:
	if(weights.size() != staticData.GetScoreIndexManager(0).GetTotalNumberOfScores()) {
	  TRACE_ERR("ERROR: " << staticData.GetScoreIndexManager(0).GetTotalNumberOfScores() << " score components, but " << weights.size() << " weights defined" << std::endl);
	  return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int Addconfig(int argc, char* argv[])
{
	// load data structures
	Parameter *parameter = new Parameter();
	if (!parameter->LoadParam(argc, argv))
	{
		parameter->Explain();
		delete parameter;
		TRACE_ERR("parameters wrong! \n");
		return EXIT_SUCCESS;
	}

	//const StaticData &staticData = StaticData::Instance();

	// precheck and prepare static data
	int id = StaticData::AddConfigStatic(parameter);
	if (id==-1)
	{
		  TRACE_ERR("Add configuration wrong! \n");
		  return EXIT_SUCCESS;
	}
	else
	{
		TRACE_ERR("------Add new configuration, id = "<< id << std::endl);
	}

	// comment them because currentId is not known until Readinput
/*	// check on weights
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
	  return EXIT_SUCCESS;//bo
	}
*/
	return EXIT_SUCCESS;
}

int Translating(string** nextCmd)
{
	int config_id=0;
	InputTag tag;
	const StaticData &staticData = StaticData::Instance();

	// set up read/writing class
	IOWrapper *ioWrapper = GetIODevice(staticData);
	if (ioWrapper == NULL)
		return EXIT_FAILURE;
	


	// read each sentence & decode
	InputType *source=0;
	size_t lineCount = 0;
	while(ReadInput(*ioWrapper,staticData.GetInputType(),source))
	{
			// note: source is only valid within this while loop!
		IFVERBOSE(1)
			ResetUserTime();

		//if this source is a command, stop translation for this time.
		tag = CheckTag(source, nextCmd, &config_id);
		if (tag == Command)
		{
		  break;
		}
		else if (tag == ConfigId)
		{
		  // configuration id
		  const_cast<ConfigurationsManager&>(staticData.GetConfigManager()).SetCurrentConfigId(config_id);
		  continue;
		}
		else if (tag == InvalidTag)
		{
		  continue;
		}	
		std::cout<<"------getcfgId="<<source->GetCfgId()<<std::endl;
const_cast<ConfigurationsManager&>(staticData.GetConfigManager()).SetCurrentConfigId(source->GetCfgId());
	// move check here, temp!!!
	vector<float> weights = staticData.GetAllWeights();
	IFVERBOSE(2) {
	  TRACE_ERR("The score component vector looks like this:\n" << staticData.GetScoreIndexManager(source->GetCfgId()));
	  TRACE_ERR("The global weight vector looks like this:");
	  for (size_t j=0; j<weights.size(); j++) { TRACE_ERR(" " << weights[j]); }
	  TRACE_ERR("\n");
	}
    VERBOSE(2,"\nTRANSLATING(" << ++lineCount << "): " << *source);

		Manager manager(*source, staticData.GetSearchAlgorithm());
		manager.ProcessSentence();

		if (staticData.GetOutputWordGraph())
			manager.GetWordGraph(source->GetTranslationId(), ioWrapper->GetOutputWordGraphStream());

                if (staticData.GetOutputSearchGraph())
		  manager.GetSearchGraph(source->GetTranslationId(), ioWrapper->GetOutputSearchGraphStream());

#ifdef HAVE_PROTOBUF
                if (staticData.GetOutputSearchGraphPB()) {
			ostringstream sfn;
			sfn << staticData.GetParam("output-search-graph-pb")[0] << '/' << source->GetTranslationId() << ".pb" << ends;
			string fn = sfn.str();
			VERBOSE(2, "Writing search graph to " << fn << endl);
			fstream output(fn.c_str(), ios::trunc | ios::binary | ios::out);
			manager.SerializeSearchGraphPB(source->GetTranslationId(), output);
		}
#endif

		// pick best translation (maximum a posteriori decoding)
		if (! staticData.UseMBR()) {
			ioWrapper->OutputBestHypo(manager.GetBestHypothesis(), source->GetTranslationId(),
						 staticData.GetReportSegmentation(), staticData.GetReportAllFactors());
			IFVERBOSE(2) { PrintUserTime("Best Hypothesis Generation Time:"); }

			// n-best
			size_t nBestSize = staticData.GetNBestSize();
			if (nBestSize > 0)
				{
			  	VERBOSE(2,"WRITING " << nBestSize << " TRANSLATION ALTERNATIVES TO " << staticData.GetNBestFilePath() << endl);
					TrellisPathList nBestList;
					manager.CalcNBest(nBestSize, nBestList,staticData.GetDistinctNBest());
					ioWrapper->OutputNBestList(nBestList, source->GetTranslationId(), source->GetCfgId());
					//RemoveAllInColl(nBestList);

					IFVERBOSE(2) { PrintUserTime("N-Best Hypotheses Generation Time:"); }
			}
		}
		// consider top candidate translations to find minimum Bayes risk translation
		else {
		  size_t nBestSize = staticData.GetMBRSize();

		  if (nBestSize <= 0)
		    {
		      cerr << "ERROR: negative size for number of MBR candidate translations not allowed (option mbr-size)" << endl;
		      return EXIT_FAILURE;
		    }
		  else
		    {
		      TrellisPathList nBestList;
		      manager.CalcNBest(nBestSize, nBestList,true);
		      VERBOSE(2,"size of n-best: " << nBestList.GetSize() << " (" << nBestSize << ")" << endl);
		      IFVERBOSE(2) { PrintUserTime("calculated n-best list for MBR decoding"); }
		      std::vector<const Factor*> mbrBestHypo = doMBR(nBestList);
		      ioWrapper->OutputBestHypo(mbrBestHypo, source->GetTranslationId(),
					       staticData.GetReportSegmentation(),
					       staticData.GetReportAllFactors());
		      IFVERBOSE(2) { PrintUserTime("finished MBR decoding"); }
		    }
		}

		if (staticData.IsDetailedTranslationReportingEnabled()) {
		  TranslationAnalysis::PrintTranslationAnalysis(std::cerr, manager.GetBestHypothesis());
		}

		IFVERBOSE(2) { PrintUserTime("Sentence Decoding Time:"); }

		manager.CalcDecoderStatistics();
	}

	delete ioWrapper;

	IFVERBOSE(1)
		PrintUserTime("End.");
	return EXIT_SUCCESS;
}


int main(int argc, char* argv[])
{
   

#ifdef HAVE_PROTOBUF
	GOOGLE_PROTOBUF_VERIFY_VERSION;
#endif
	string* cmd = new string("");
	int res;

	IFVERBOSE(1)
	{
		TRACE_ERR("command: ");
		for(int i=0;i<argc;++i) TRACE_ERR(argv[i]<<" ");
		TRACE_ERR(endl);
	}

	cout.setf(std::ios::fixed);
	cout.precision(3);
	cerr.setf(std::ios::fixed);
	cerr.precision(3);

	// load data structures
	Parameter *parameter = new Parameter();
	if (!parameter->LoadParam(argc, argv))
	{
		parameter->Explain();
		delete parameter;
		exit(EXIT_FAILURE);
	}

	const StaticData &staticData = StaticData::Instance();

	if (!StaticData::LoadDataStatic(parameter))
		exit(EXIT_FAILURE);

	// check on weights
	vector<float> weights = staticData.GetAllWeights();
	IFVERBOSE(2) {
	  TRACE_ERR("The score component vector looks like this:\n" << staticData.GetScoreIndexManager(0));
	  TRACE_ERR("The global weight vector looks like this:");
	  for (size_t j=0; j<weights.size(); j++) { TRACE_ERR(" " << weights[j]); }
	  TRACE_ERR("\n");
	}
	// every score must have a weight!  check that here:
	if(weights.size() != staticData.GetScoreIndexManager(0).GetTotalNumberOfScores()) {
	  TRACE_ERR("ERROR: " << staticData.GetScoreIndexManager(0).GetTotalNumberOfScores() << " score components, but " << weights.size() << " weights defined" << std::endl);
	  exit(EXIT_FAILURE);
	}

  do{
	if ( cmd->compare("") ==0 )
	{
	  res = Translating(&cmd);
	}
	else
	{
	  int arg_count;
	  vector<string> arguments;

	  ParseCommand(cmd, &arg_count, &arguments);

	  char* args[arg_count];
	  for (int i=0; i<arg_count; i++)
	  {
		args[i] = (char*)arguments.at(i).c_str();	
	  }

	  VERBOSE(1, "going to ProcessCommand, arguments: "<< " ");
	  for (int i=0; i<arg_count; i++)
	  {	
		VERBOSE(1,args[i] << " ");
	  }
	  VERBOSE(1,endl);

	  //if (string(args[0]).compare("#config")==0)
	  if (string(args[0]).compare("reconfig")==0)
	  {
	    res = Reconfig(arg_count, args);
	    *cmd = "";
	  }
	  else if (string(args[0]).compare("addconfig")==0)
	  {
	    res = Addconfig(arg_count, args);
	    *cmd = "";
	  }
	  else
	  {
	    VERBOSE(1,"command not supported." << endl);
	    res = EXIT_SUCCESS;
	    *cmd = "";
	  }
	}

	if (res != EXIT_SUCCESS)
	{
	  #ifndef EXIT_RETURN
	  //This avoids that detructors are called (it can take a long time)
		exit(res);
	  #else
		return res;
	  #endif
	}

  }while (!CommandIsQuit(cmd));

	#ifndef EXIT_RETURN
	//This avoids that detructors are called (it can take a long time)
		exit(EXIT_SUCCESS);
	#else
		return EXIT_SUCCESS;
	#endif
}


