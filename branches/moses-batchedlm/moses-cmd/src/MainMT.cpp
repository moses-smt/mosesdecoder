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


/**
  * Makes sure output goes in the correct order.
  **/
class OutputCollector {
    public:
        OutputCollector(std::ostream* outStream= &cout, std::ostream* debugStream=&cerr) :
            m_nextOutput(0),m_outStream(outStream),m_debugStream(debugStream)  {}


        /**
          * Write or cache the output, as appropriate.
          **/
        void Write(int sourceId,const string& output,const string& debug="") {
            boost::mutex::scoped_lock lock(m_mutex);
            if (sourceId == m_nextOutput) {
                //This is the one we were expecting
                *m_outStream << output;
                *m_debugStream << debug;
                ++m_nextOutput;
                //see if there's any more
                map<int,string>::iterator iter;
                while ((iter = m_outputs.find(m_nextOutput)) != m_outputs.end()) {
                    *m_outStream << iter->second;
                    m_outputs.erase(iter);
                    ++m_nextOutput;
                    map<int,string>::iterator debugIter = m_debugs.find(iter->first);
                    if (debugIter != m_debugs.end()) {
                      *m_debugStream << debugIter->second;
                      m_debugs.erase(debugIter);
                    }
                }
            } else {
                //save for later
                m_outputs[sourceId] = output;
                m_debugs[sourceId] = debug;
            }
        }
        
     private:
        map<int,string> m_outputs;
        map<int,string> m_debugs;
        int m_nextOutput;
        ostream* m_outStream;
        ostream* m_debugStream;
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
            Sentence sentence(Input);
            Manager manager(*m_source,staticData.GetSearchAlgorithm());
            manager.ProcessSentence();
            
            if (m_outputCollector) {
                ostringstream out;
                ostringstream debug;
                const Hypothesis* bestHypo = NULL;
                if (!staticData.UseMBR()) {
                    bestHypo = manager.GetBestHypothesis();
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
                        bestHypo = doMBR(nBestList)->GetEdges().at(0);
                        IFVERBOSE(2) { PrintUserTime("finished MBR decoding"); }
                    }
                }
                if (bestHypo) {
                    OutputSurface(
                            out,
                            bestHypo,
                            staticData.GetOutputFactorOrder(), 
                            staticData.GetReportSegmentation(),
                            staticData.GetReportAllFactors());
                    IFVERBOSE(1) {
                      debug << "BEST TRANSLATION: " << *bestHypo << endl;
                    }
                }
                out << endl;

                m_outputCollector->Write(m_lineNumber,out.str(),debug.str());
            }
            if (m_nbestCollector) {
                TrellisPathList nBestList;
                ostringstream out;
                manager.CalcNBest(staticData.GetNBestSize(), nBestList,staticData.GetDistinctNBest());
                OutputNBest(out,nBestList, staticData.GetOutputFactorOrder(), m_lineNumber);
                m_nbestCollector->Write(m_lineNumber, out.str());
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
        TranslationTask* task = 
            new TranslationTask(lineCount,source, outputCollector.get(), nbestCollector.get());
        pool.Submit(task);
        source = NULL; //make sure it doesn't get deleted
        ++lineCount;
    }

    pool.Stop(true); //flush remaining jobs
    return 0;
}



