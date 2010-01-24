// $Id:  $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

/**
 * Main for multithreaded moses.
 **/

#include <sstream>
#include <vector>
#include <string> 

#include <boost/thread/mutex.hpp>

#if defined(BOOST_HAS_PTHREADS)
#include <pthread.h>
#endif


#include "Hypothesis.h"
#include "IOWrapper.h"
#include "Manager.h"
#include "StaticData.h"
#include "ThreadPool.h"
#include "Util.h"
#include "mbr.h"

using namespace std;
using namespace Moses;

bool CheckCommand(const InputType *line, string** cmd)
{
	stringstream stream;
	string str;
	stream << *line;
	str = stream.str();

	if ( str.find("<decoder ") == 0 )
	{
	  // remove last char due to the stream
	  str = str.substr(0,(str.size()-1));
	  *cmd = new string(str.c_str());
	  VERBOSE(1, "detected a command: " << **cmd << endl);
	  return true;
	} 
	else
	{
	  return false;
	}
}

void ParseCommand(string* cmd, int* argc, vector<string>* argv)
{
	*argc=0;
	argv->clear();
	size_t len = 0;
	cmd->erase(cmd->find_last_not_of(" ")+1);// strip

        if (cmd->find("<decoder quit")!= string::npos)
        {
            (*argc)++;
	    argv->push_back("quit");
            return;
        }

	// <decoder addconfig="...">
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

	return EXIT_SUCCESS;
}

/**
  * Makes sure output goes in the correct order.
  **/
class OutputCollector {
    public:
        OutputCollector(std::ostream* outStream= &cout) :
            m_nextOutput(0),m_outStream(outStream)  {}


        /**
          * Write or cache the output, as appropriate.
          **/
        void Write(int sourceId,const string& output) {
            boost::mutex::scoped_lock lock(m_mutex);
            if (sourceId == m_nextOutput) {
                //This is the one we were expecting
                *m_outStream << output;
                ++m_nextOutput;
                //see if there's any more
                map<int,string>::iterator iter;
                while ((iter = m_outputs.find(m_nextOutput)) != m_outputs.end()) {
                    *m_outStream << iter->second;
                    m_outputs.erase(iter);
                    ++m_nextOutput;
                }
            } else {
                //save for later
                m_outputs[sourceId] = output;
            }
        }
        
     private:
        map<int,string> m_outputs;
        int m_nextOutput;
        ostream* m_outStream;
        boost::mutex m_mutex;
};

/**
  * Translates a sentence.
  **/
class TranslationTask : public Task {

    public:

        TranslationTask(size_t lineNumber,
             InputType* source, OutputCollector* outputCollector, OutputCollector* nbestCollector) :
             m_source(source), m_lineNumber(lineNumber),
                m_outputCollector(outputCollector), m_nbestCollector(nbestCollector) {}

        void Run() {
#if defined(BOOST_HAS_PTHREADS)
            TRACE_ERR("Translating line " << m_lineNumber << "  in thread id " << (int)pthread_self() << std::endl);
#endif
            const StaticData &staticData = StaticData::Instance();
    if (staticData.IsValidId(m_source->GetCfgId()))
    {

            Sentence sentence(Input);
            Manager manager(*m_source,staticData.GetSearchAlgorithm(m_source->GetCfgId()));
            manager.ProcessSentence();
            
            if (m_outputCollector) {
                ostringstream out;
                if (!staticData.UseMBR()) {
                    const Hypothesis* hypo = manager.GetBestHypothesis();
                    if (hypo) {
                        OutputSurface(
                                out,
                                hypo,
                                staticData.GetOutputFactorOrder(), 
                                staticData.GetReportSegmentation(),
                                staticData.GetReportAllFactors());
                    }
                    out << endl;
                } else {
                    //MBR decoding
                    size_t nBestSize = staticData.GetMBRSize();
                    if (nBestSize <= 0) {
                        cerr << "ERROR: negative size for number of MBR candidate translations not allowed (option mbr-size)" << endl;
                        exit(1);
                    } else {
                        TrellisPathList nBestList;
                        manager.CalcNBest(nBestSize, nBestList,true);
                        VERBOSE(2,"size of n-best: " << nBestList.GetSize() << " (" << nBestSize << ")" << endl);
                        IFVERBOSE(2) { PrintUserTime("calculated n-best list for MBR decoding"); }
                        vector<const Factor*> mbrBestHypo = doMBR(nBestList);
                        for (size_t i = 0 ; i < mbrBestHypo.size() ; i++) {
                            const Factor *factor = mbrBestHypo[i];
                            if (i>0) out << " ";
                            out << factor->GetString();
                        }
                        out << endl;
                        IFVERBOSE(2) { PrintUserTime("finished MBR decoding"); }
                    }
                }
                m_outputCollector->Write(m_lineNumber,out.str());
            }
            if (m_nbestCollector) {
                TrellisPathList nBestList;
                ostringstream out;
                manager.CalcNBest(staticData.GetNBestSize(), nBestList,staticData.GetDistinctNBest());
                OutputNBest(out,nBestList, staticData.GetOutputFactorOrder(), m_lineNumber,m_source->GetCfgId());
                m_nbestCollector->Write(m_lineNumber, out.str());
            }
    }
    else{  
            TRACE_ERR("------invalid cfg id" << std::endl);  
            // more process here!!!
        }
        }

        ~TranslationTask() {delete m_source;}

    private:
        InputType* m_source;
        size_t m_lineNumber;
        OutputCollector* m_outputCollector;
        OutputCollector* m_nbestCollector;

};

int main(int argc, char** argv) {
    //extract pool-size args, send others to moses
    char** mosesargv = new char*[argc+2];
    int mosesargc = 0;
    int threadcount = 10;
    string* cmd = new string("");
    int arg_count;
    vector<string> arguments;

    for (int i = 0; i < argc; ++i) {
        if (!strcmp(argv[i], "-threads")) {
            ++i;
            if (i >= argc) {
                cerr << "Error: Missing argument to -threads" << endl;
                exit(1);
            } else {
                threadcount = atoi(argv[i]);
            }
        } else {
            mosesargv[mosesargc] = new char[strlen(argv[i])+1];
            strcpy(mosesargv[mosesargc],argv[i]);
            ++mosesargc;
        }
    }
    if (threadcount <= 0) {
        cerr << "Error: Must specify a positive number of threads" << endl;
        exit(1);
    }

    Parameter* params = new Parameter();
    if (!params->LoadParam(mosesargc,mosesargv)) {
        params->Explain();
        exit(1);
    }
    if (!StaticData::LoadDataStatic(params)) {
        exit(1);
    }

    const StaticData& staticData = StaticData::Instance();
    IOWrapper* ioWrapper = GetIODevice(staticData);

    if (!ioWrapper) {
        cerr << "Error; Failed to create IO object" << endl;
        exit(1);
    }
    ThreadPool pool(threadcount);
    InputType* source = NULL;
    size_t lineCount = 0;
    auto_ptr<OutputCollector> outputCollector;//for translations
    auto_ptr<OutputCollector> nbestCollector;
    auto_ptr<ofstream> nbestOut;
    size_t nbestSize = staticData.GetNBestSize();
    string nbestFile = staticData.GetNBestFilePath();
    if (nbestSize) {
        if (nbestFile == "-") {
            //nbest to stdout, no 1-best
            //FIXME: Moses doesn't actually let you pass a '-' on the command line.
            nbestCollector.reset(new OutputCollector());
        } else {
            //nbest to file, 1-best to stdout
            nbestOut.reset(new ofstream(nbestFile.c_str()));
            assert(nbestOut->good());
            nbestCollector.reset(new OutputCollector(nbestOut.get()));
            outputCollector.reset(new OutputCollector());
        }
    } else {
        outputCollector.reset(new OutputCollector());
    }
    
	while(ReadInput(*ioWrapper,staticData.GetInputType(),source)) {
        // command is processed in this thread
        if ( CheckCommand(source, &cmd) ) {
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

	    if (string(args[0]).compare("quit")==0)
	    {
	        *cmd = "";
                break;
	    }
	    else if (string(args[0]).compare("addconfig")==0)
	    {
	        Addconfig(arg_count, args);
	        *cmd = "";
                continue;
	    }
	    else
	    {
	        VERBOSE(1,"command not supported." << endl);
	        *cmd = "";
                continue;
	    }
        }

        TranslationTask* task = 
            new TranslationTask(lineCount,source, outputCollector.get(), nbestCollector.get());
        pool.Submit(task);
        source = NULL; //make sure it doesn't get deleted
        ++lineCount;
    }

    pool.Stop(true); //flush remaining jobs
    return 0;
}



