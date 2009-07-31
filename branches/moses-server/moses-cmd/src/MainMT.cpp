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
        OutputCollector() :
            m_nextOutput(0)  {}

        void Write(int sourceId, Manager& manager) {
            //create the output string
            //Note that this is copied from Main.cpp. Some refactoring
            //could remove the duplicate code.
            const StaticData& staticData = StaticData::Instance();
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
        Write(sourceId,out.str());
    }

    private:
        /**
          * Write or cache the output, as appropriate.
          **/
        void Write(int sourceId,const string& output) {
            boost::mutex::scoped_lock lock(m_mutex);
            if (sourceId == m_nextOutput) {
                //This is the one we were expecting
                cout << output;
                ++m_nextOutput;
                //see if there's any more
                map<int,string>::iterator iter;
                while ((iter = m_outputs.find(m_nextOutput)) != m_outputs.end()) {
                    cout << iter->second;
                    m_outputs.erase(iter);
                    ++m_nextOutput;
                }
            } else {
                //save for later
                m_outputs[sourceId] = output;
            }
        }
        map<int,string> m_outputs;
        int m_nextOutput;
        boost::mutex m_mutex;
};

/**
  * Translates a sentence.
  **/
class TranslationTask : public Task {

    public:

        TranslationTask(size_t lineNumber,
             InputType* source, OutputCollector& outputCollector) :
             m_source(source), m_lineNumber(lineNumber),
                m_outputCollector(outputCollector) {}

        void Run() {
#if defined(BOOST_HAS_PTHREADS)
            TRACE_ERR("Translating line " << m_lineNumber << "  in thread id " << (int)pthread_self() << std::endl);
#endif
            const StaticData &staticData = StaticData::Instance();
            Sentence sentence(Input);
            const vector<FactorType> &inputFactorOrder = 
                staticData.GetInputFactorOrder();
            Manager manager(*m_source,staticData.GetSearchAlgorithm());
            manager.ProcessSentence();
            m_outputCollector.Write(m_lineNumber,manager);
        }

        ~TranslationTask() {delete m_source;}

    private:
        InputType* m_source;
        size_t m_lineNumber;
        OutputCollector& m_outputCollector;

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
    OutputCollector outputCollector;
	while(ReadInput(*ioWrapper,staticData.GetInputType(),source)) {
        TranslationTask* task = 
            new TranslationTask(lineCount,source, outputCollector);
        pool.Submit(task);
        source = NULL; //make sure it doesn't get deleted
        ++lineCount;
    }

    pool.Stop(true); //flush remaining jobs
    return 0;
}



