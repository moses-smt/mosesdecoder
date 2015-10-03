#include <iostream>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "NMT_Wrapper.h"

typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager> CharAllocator;
typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator> ShmemString;
typedef boost::interprocess::allocator<ShmemString, boost::interprocess::managed_shared_memory::segment_manager> ShmemStringAllocator;

typedef boost::interprocess::allocator<float, boost::interprocess::managed_shared_memory::segment_manager> ShmemFloatAllocator;
typedef boost::interprocess::allocator<void*, boost::interprocess::managed_shared_memory::segment_manager> ShmemVoidptrAllocator;
 
typedef std::vector<ShmemString, ShmemStringAllocator> ShmemStringVector;
typedef std::vector<float, ShmemFloatAllocator> ShmemFloatVector;
typedef std::vector<void*, ShmemVoidptrAllocator> ShmemVoidptrVector;

using namespace std;

struct _object;
typedef _object PyObject;

const int NCOPY = 10;

int main(int argc, char *argv[])
{
    string statePath = string(argv[1]);
    string modelPath = string(argv[2]);
    string wrapperPath = string(argv[3]);
    string sourceVocab = string(argv[4]);
    string targetVocab = string(argv[5]);

    std::cerr << "Waiting 1" << std::endl;
    
    boost::interprocess::named_mutex m_mutex(boost::interprocess::open_or_create, "MyMutex");
    std::cerr << "Waiting 2" << std::endl;
    boost::interprocess::named_condition m_moses(boost::interprocess::open_or_create, "mosesCondition");
    std::cerr << "Waiting 3" << std::endl;
    boost::interprocess::named_condition m_neural(boost::interprocess::open_or_create,"neuralCondition");
    std::cerr << "Waiting 4" << std::endl;
    
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(m_mutex);
    std::cerr << "Waiting" << std::endl;
    m_neural.wait(lock);
    std::cerr << "Done" << std::endl;
    
    boost::interprocess::managed_shared_memory m_segment(boost::interprocess::open_or_create ,"NeuralSharedMemory", 1024*1024*1024);
    
    std::pair<void**, size_t> res1 = m_segment.find<void*>("NeuralContextPtr");
    void** shPyContextVectors = res1.first;
  
    CharAllocator charAlloc(m_segment.get_segment_manager());
    std::pair<ShmemString*, size_t> res2 = m_segment.find<ShmemString>("NeuralContextString");
    ShmemString* sharedSentence = res2.first;
    
    string sourceSentence(sharedSentence->begin(), sharedSentence->end());
    std::cout << "Received string: " << sourceSentence << std::endl;
          
    boost::shared_ptr<NMT_Wrapper> wrapper(new NMT_Wrapper());
    wrapper->Init(statePath, modelPath, wrapperPath, sourceVocab, targetVocab);

    PyObject* pyContextVectors = NULL;
    wrapper->GetContextVectors(sourceSentence, pyContextVectors);

    *shPyContextVectors = pyContextVectors;
    
    while(1) {
        m_moses.notify_one();
        m_neural.wait(lock);
      
        std::cerr << "Calculation" << std::endl;
        std::pair<ShmemStringVector*, size_t > v1 = m_segment.find<ShmemStringVector>("NeuralAllWords");
        std::pair<ShmemStringVector*, size_t > v2 = m_segment.find<ShmemStringVector>("NeuralAllLastWords");
        std::pair<ShmemVoidptrVector*, size_t > v3 = m_segment.find<ShmemVoidptrVector>("NeuralAllStates");
        
        std::vector<std::string> allWords;
        std::vector<std::string> allLastWords;
        std::vector<PyObject*> allStates;
        
        for(size_t i = 0; i < v1.first->size(); ++i) {
            allWords.push_back(std::string((*v1.first)[i].begin(), (*v1.first)[i].end()));
            allLastWords.push_back(std::string((*v2.first)[i].begin(), (*v2.first)[i].end()));
            allStates.push_back((PyObject*)(*v3.first)[i]);
        }
        
        std::vector<double> outProbs;
        std::vector<PyObject*> allOutStates;
        
        wrapper->GetNextLogProbStates(allWords, pyContextVectors, allLastWords,
                                      allStates, outProbs, allOutStates);
        
        std::pair<ShmemVoidptrVector*, size_t > v4 = m_segment.find<ShmemVoidptrVector>("NeuralAllOutStates");
        std::pair<ShmemFloatVector*, size_t > v5 = m_segment.find<ShmemFloatVector>("NeuralLogProbs");
        
        for(size_t i = 0; i < outProbs.size(); ++i) {
            v4.first->push_back(allOutStates[i]);
            v5.first->push_back(outProbs[i]);
        }
        
        std::cerr << "Done" << std::endl;
    }
        
    //vector<double> prob;
    //vector<PyObject*> nextStates;
    //vector<string> nextWords;
    //for (size_t i = 0; i < 1000; ++i) nextWords.push_back("das");
    //
    //vector<string> lastWords;
    //for (size_t i = 0; i < 1000; ++i) lastWords.push_back("");
    //vector<PyObject*> inputStates;
    //for (size_t i = 0; i < 1000; ++i) inputStates.push_back(NULL);
    //

    /*
    wrapper->GetNextLogProbStates(nextWords, pyContextVectors, lastWords,
                                  inputStates, prob, nextStates);
    cout << prob.size() << " " << prob[0] << endl;
    */
    
    return 0;
}
