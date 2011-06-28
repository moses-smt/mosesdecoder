// $Id: MainMT.cpp 3045 2010-04-05 13:07:29Z hieuhoang1972 $

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
 * Moses main, for single-threaded and multi-threaded.
 **/

#include <fstream>
#include <sstream>
#include <vector>

#ifdef WIN32
// Include Visual Leak Detector
#include <vld.h>
#endif

#ifdef WITH_THREADS
#include <boost/thread/mutex.hpp>
#endif

#ifdef BOOST_HAS_PTHREADS
#include <pthread.h>
#endif

#include "Hypothesis.h"
#include "IOWrapper.h"
#include "LatticeMBR.h"
#include "Manager.h"
#include "StaticData.h"
#include "Util.h"
#include "mbr.h"
#include "ThreadPool.h"
#include "TranslationAnalysis.h"

#ifdef HAVE_PROTOBUF
#include "hypergraph.pb.h"
#endif

using namespace std;
using namespace Moses;

static const size_t PRECISION = 3;

/** Enforce rounding */
void fix(std::ostream& stream, size_t size) {
    stream.setf(std::ios::fixed);
    stream.precision(size);
}


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
#ifdef WITH_THREADS
            boost::mutex::scoped_lock lock(m_mutex);
#endif
            if (sourceId == m_nextOutput) {
                //This is the one we were expecting
                *m_outStream << output << flush;
                *m_debugStream << debug << flush;
                ++m_nextOutput;
                //see if there's any more
                map<int,string>::iterator iter;
                while ((iter = m_outputs.find(m_nextOutput)) != m_outputs.end()) {
                    *m_outStream << iter->second << flush;
                    m_outputs.erase(iter);
                    ++m_nextOutput;
                    map<int,string>::iterator debugIter = m_debugs.find(iter->first);
                    if (debugIter != m_debugs.end()) {
                      *m_debugStream << debugIter->second << flush;
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
#ifdef WITH_THREADS
        boost::mutex m_mutex;
#endif
};

/**
  * Translates a sentence.
  **/
class TranslationTask : public Task {

    public:

        TranslationTask(size_t lineNumber,
             InputType* source, OutputCollector* outputCollector, OutputCollector* nbestCollector,
                       OutputCollector* wordGraphCollector, OutputCollector* searchGraphCollector,
                       OutputCollector* detailedTranslationCollector) :
             m_source(source), m_lineNumber(lineNumber),
                m_outputCollector(outputCollector), m_nbestCollector(nbestCollector),
                m_wordGraphCollector(wordGraphCollector), m_searchGraphCollector(searchGraphCollector),
                m_detailedTranslationCollector(detailedTranslationCollector) {}

        void Run() 
        {
#ifdef BOOST_HAS_PTHREADS
            TRACE_ERR("Translating line " << m_lineNumber << "  in thread id " << pthread_self() << std::endl);
#endif
            const StaticData &staticData = StaticData::Instance();
            Sentence sentence(Input);
            const TranslationSystem& system = staticData.GetTranslationSystem(TranslationSystem::DEFAULT);
            Manager manager(*m_source,staticData.GetSearchAlgorithm(), &system);
            manager.ProcessSentence();
                        
            //Word Graph
            if (m_wordGraphCollector) {
                ostringstream out;
                fix(out,PRECISION);
                manager.GetWordGraph(m_lineNumber, out);
                m_wordGraphCollector->Write(m_lineNumber, out.str());
            }
            
            //Search Graph
            if (m_searchGraphCollector) {
                ostringstream out;
                fix(out,PRECISION);
                manager.OutputSearchGraph(m_lineNumber, out);
                m_searchGraphCollector->Write(m_lineNumber, out.str());

#ifdef HAVE_PROTOBUF
                if (staticData.GetOutputSearchGraphPB()) {
                    ostringstream sfn;
                    sfn << staticData.GetParam("output-search-graph-pb")[0] << '/' << m_lineNumber << ".pb" << ends;
                    string fn = sfn.str();
                    VERBOSE(2, "Writing search graph to " << fn << endl);
                    fstream output(fn.c_str(), ios::trunc | ios::binary | ios::out);
                    manager.SerializeSearchGraphPB(m_lineNumber, output);
                }
#endif
            }
            
            
            
            if (m_outputCollector) {
                ostringstream out;
                ostringstream debug;
                fix(debug,PRECISION);
                
                //All derivations - send them to debug stream
                if (staticData.PrintAllDerivations()) {
                    manager.PrintAllDerivations(m_lineNumber, debug);
                }
                
                //Best hypothesis
                const Hypothesis* bestHypo = NULL;
                if (!staticData.UseMBR()) {
                    bestHypo = manager.GetBestHypothesis();
                    if (bestHypo) {
                        if (staticData.IsPathRecoveryEnabled()) {
                            OutputInput(out, bestHypo);
                            out << "||| ";
                        }
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
                } 
                else 
                {
                    size_t nBestSize = staticData.GetMBRSize();
                    if (nBestSize <= 0) 
                    {
                        cerr << "ERROR: negative size for number of MBR candidate translations not allowed (option mbr-size)" << endl;
                        exit(1);
                    }
                    TrellisPathList nBestList;
                    manager.CalcNBest(nBestSize, nBestList,true);
                    VERBOSE(2,"size of n-best: " << nBestList.GetSize() << " (" << nBestSize << ")" << endl);
                    IFVERBOSE(2) { PrintUserTime("calculated n-best list for (L)MBR decoding"); }
                    
                    if (staticData.UseLatticeMBR()) 
                    {
                        if (m_nbestCollector) 
                        {
                            //lattice mbr nbest
                            vector<LatticeMBRSolution> solutions;
                            size_t n  = min(nBestSize, staticData.GetNBestSize());
                            getLatticeMBRNBest(manager,nBestList,solutions,n);
                            ostringstream out;
                            OutputLatticeMBRNBest(out, solutions,m_lineNumber);
                            m_nbestCollector->Write(m_lineNumber, out.str());
                        } 
                        else 
                        {
                            //Lattice MBR decoding
                            vector<Word> mbrBestHypo = doLatticeMBR(manager,nBestList); 
                            OutputBestHypo(mbrBestHypo, m_lineNumber, staticData.GetReportSegmentation(),
                                        staticData.GetReportAllFactors(),out);
                            IFVERBOSE(2) { PrintUserTime("finished Lattice MBR decoding"); }
                        }
                    } 
                    else if (staticData.UseConsensusDecoding()) {
                        const TrellisPath &conBestHypo = doConsensusDecoding(manager,nBestList);
                        OutputBestHypo(conBestHypo, m_lineNumber,
                                       staticData.GetReportSegmentation(),
                                       staticData.GetReportAllFactors(),out);
                        IFVERBOSE(2) { PrintUserTime("finished Consensus decoding"); }
                    } 
                    else
                    {
                        //MBR decoding
					    const Moses::TrellisPath &mbrBestHypo = doMBR(nBestList);
                        OutputBestHypo(mbrBestHypo, m_lineNumber,
                                    staticData.GetReportSegmentation(),
                                    staticData.GetReportAllFactors(),out);
                        IFVERBOSE(2) { PrintUserTime("finished MBR decoding"); }
                        
                    }
                }
                m_outputCollector->Write(m_lineNumber,out.str(),debug.str());
            }
            if (m_nbestCollector && !staticData.UseLatticeMBR()) {
                TrellisPathList nBestList;
                ostringstream out;
                manager.CalcNBest(staticData.GetNBestSize(), nBestList,staticData.GetDistinctNBest());
                OutputNBest(out,nBestList, staticData.GetOutputFactorOrder(), manager.GetTranslationSystem(), m_lineNumber);
                m_nbestCollector->Write(m_lineNumber, out.str());
            }
            
            //detailed translation reporting
            if (m_detailedTranslationCollector) {
								ostringstream out;
                fix(out,PRECISION);
                TranslationAnalysis::PrintTranslationAnalysis(manager.GetTranslationSystem(), out, manager.GetBestHypothesis());
                m_detailedTranslationCollector->Write(m_lineNumber,out.str());
            }

            IFVERBOSE(2) { PrintUserTime("Sentence Decoding Time:"); }

            manager.CalcDecoderStatistics();
        }

        ~TranslationTask() {delete m_source;}

    private:
        InputType* m_source;
        size_t m_lineNumber;
        OutputCollector* m_outputCollector;
        OutputCollector* m_nbestCollector;
        OutputCollector* m_wordGraphCollector;
        OutputCollector* m_searchGraphCollector;
        OutputCollector* m_detailedTranslationCollector;
        

};

static void PrintFeatureWeight(const FeatureFunction* ff) {
  
  size_t weightStart  = StaticData::Instance().GetScoreIndexManager().GetBeginIndex(ff->GetScoreBookkeepingID());
  size_t weightEnd  = StaticData::Instance().GetScoreIndexManager().GetEndIndex(ff->GetScoreBookkeepingID());
  for (size_t i = weightStart; i < weightEnd; ++i) {
    cout << ff->GetScoreProducerDescription() <<  " " << ff->GetScoreProducerWeightShortName() << " " 
        << StaticData::Instance().GetAllWeights()[i] << endl;
  }
}


static void ShowWeights() {
  fix(cout,6);
  const StaticData& staticData = StaticData::Instance();
  const TranslationSystem& system = staticData.GetTranslationSystem(TranslationSystem::DEFAULT);
  const vector<const StatelessFeatureFunction*>& slf =system.GetStatelessFeatureFunctions();
  const vector<const StatefulFeatureFunction*>& sff = system.GetStatefulFeatureFunctions();
  const vector<PhraseDictionaryFeature*>& pds = system.GetPhraseDictionaries();
  const vector<GenerationDictionary*>& gds = system.GetGenerationDictionaries();
  for (size_t i = 0; i < sff.size(); ++i) {
    PrintFeatureWeight(sff[i]);
  }
  for (size_t i = 0; i < slf.size(); ++i) {
    PrintFeatureWeight(slf[i]);
  }
  for (size_t i = 0; i < pds.size(); ++i) {
    PrintFeatureWeight(pds[i]);
  }
  for (size_t i = 0; i < gds.size(); ++i) {
    PrintFeatureWeight(gds[i]);
  }
}

int main(int argc, char** argv) {
    
#ifdef HAVE_PROTOBUF
    GOOGLE_PROTOBUF_VERIFY_VERSION;
#endif
    IFVERBOSE(1)
    {
        TRACE_ERR("command: ");
        for(int i=0;i<argc;++i) TRACE_ERR(argv[i]<<" ");
        TRACE_ERR(endl);
    }

    fix(cout,PRECISION);
    fix(cerr,PRECISION);


    Parameter* params = new Parameter();
    if (!params->LoadParam(argc,argv)) {
        params->Explain();
        exit(1);
    }
    
    //create threadpool, if necessary
    int threadcount = (params->GetParam("threads").size() > 0) ?
            Scan<size_t>(params->GetParam("threads")[0]) : 1;
    
#ifdef WITH_THREADS
    if (threadcount < 1) {
        cerr << "Error: Need to specify a positive number of threads" << endl;
        exit(1);
    }
    ThreadPool pool(threadcount);
#else
    if (threadcount > 1) {
        cerr << "Error: Thread count of " << threadcount << " but moses not built with thread support" << endl;
        exit(1);
    }
#endif

    
    if (!StaticData::LoadDataStatic(params)) {
        exit(1);
    }
    
    if (params->isParamSpecified("show-weights")) {
      ShowWeights();
      exit(0);
    }

    const StaticData& staticData = StaticData::Instance();
    // set up read/writing class
    IOWrapper* ioWrapper = GetIODevice(staticData);

    if (!ioWrapper) {
        cerr << "Error; Failed to create IO object" << endl;
        exit(1);
    }
    
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
        exit(1);
    }
    

    InputType* source = NULL;
    size_t lineCount = 0;
    auto_ptr<OutputCollector> outputCollector;//for translations
    auto_ptr<OutputCollector> nbestCollector;
    auto_ptr<ofstream> nbestOut;
    size_t nbestSize = staticData.GetNBestSize();
    string nbestFile = staticData.GetNBestFilePath();
    if (nbestSize) {
        if (nbestFile == "-" || nbestFile == "/dev/stdout") {
            //nbest to stdout, no 1-best
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
    
    auto_ptr<OutputCollector> wordGraphCollector;
    if (staticData.GetOutputWordGraph()) {
        wordGraphCollector.reset(new OutputCollector(&(ioWrapper->GetOutputWordGraphStream())));
    }
    
    auto_ptr<OutputCollector> searchGraphCollector;
    if (staticData.GetOutputSearchGraph()) {
        searchGraphCollector.reset(new OutputCollector(&(ioWrapper->GetOutputSearchGraphStream())));
    }
    
    auto_ptr<OutputCollector> detailedTranslationCollector;
    if (staticData.IsDetailedTranslationReportingEnabled()) {
        detailedTranslationCollector.reset(new OutputCollector(&(ioWrapper->GetDetailedTranslationReportingStream())));
    }
    
	while(ReadInput(*ioWrapper,staticData.GetInputType(),source)) {
        IFVERBOSE(1) {
                ResetUserTime();
        }
        TranslationTask* task = 
                new TranslationTask(lineCount,source, outputCollector.get(), nbestCollector.get(), wordGraphCollector.get(),
                                    searchGraphCollector.get(), detailedTranslationCollector.get());
#ifdef WITH_THREADS
        pool.Submit(task);

#else
        task->Run();
#endif
        source = NULL; //make sure it doesn't get deleted
        ++lineCount;
    }
#ifdef WITH_THREADS
    pool.Stop(true); //flush remaining jobs
#endif

#ifndef EXIT_RETURN
		//This avoids that destructors are called (it can take a long time)
		exit(EXIT_SUCCESS);
#else
		return EXIT_SUCCESS;
#endif
}




