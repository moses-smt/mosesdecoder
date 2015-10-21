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
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "NMT_Wrapper.h"

namespace bi = boost::interprocess;

typedef bi::allocator<char, bi::managed_shared_memory::segment_manager> CharAllocator;
typedef bi::basic_string<char, std::char_traits<char>, CharAllocator> ShmemString;
typedef bi::allocator<ShmemString, bi::managed_shared_memory::segment_manager> ShmemStringAllocator;

typedef bi::allocator<float, bi::managed_shared_memory::segment_manager> ShmemFloatAllocator;
typedef bi::allocator<void*, bi::managed_shared_memory::segment_manager> ShmemVoidptrAllocator;
 
typedef std::vector<ShmemString, ShmemStringAllocator> ShmemStringVector;
typedef std::vector<float, ShmemFloatAllocator> ShmemFloatVector;
typedef std::vector<void*, ShmemVoidptrAllocator> ShmemVoidptrVector;

using namespace std;

struct _object;
typedef _object PyObject;


bi::interprocess_mutex& GetMutex(bi::managed_shared_memory& segment) {
    return *segment.find<bi::interprocess_mutex>("Mutex").first;
}

void NotifyParent(bi::managed_shared_memory& segment) {
    segment.find<bi::interprocess_condition>("ParentCondition").first->notify_one();
}

void WaitForParent(bi::managed_shared_memory& segment, bi::scoped_lock<bi::interprocess_mutex>& lock) {
    std::pair<bi::interprocess_condition*, size_t > p2 = segment.find<bi::interprocess_condition>("ChildCondition");
    p2.first->wait(lock);
}

void GetPaths(std::string& statePath, std::string& modelPath,
              std::string& wrapperPath, std::string& sourceVocab,
              std::string& targetVocab, 
              bi::managed_shared_memory& segment) {
    
    std::pair<ShmemString*, size_t> p1 = segment.find<ShmemString>("StatePath");
    statePath = p1.first->c_str();
    
    std::pair<ShmemString*, size_t> p2 = segment.find<ShmemString>("ModelPath");
    modelPath = p2.first->c_str();
    
    std::pair<ShmemString*, size_t> p3 = segment.find<ShmemString>("WrapperPath");
    wrapperPath = p3.first->c_str();
    
    std::pair<ShmemString*, size_t> p4 = segment.find<ShmemString>("SourceVocab");
    sourceVocab = p4.first->c_str();
    
    std::pair<ShmemString*, size_t> p5 = segment.find<ShmemString>("TargetVocab");
    targetVocab = p5.first->c_str();
}

enum State { EmptyHypothesis, InSentence, Exit };

State GetState(bi::managed_shared_memory& segment) {
    std::pair<State*, size_t> p1 = segment.find<State>("State");
    return *p1.first;
}

void HandleEmptyHypothesis(NMT_Wrapper& nmt,
                           bi::managed_shared_memory& segment) {
    std::pair<ShmemString*, size_t> p1 = segment.find<ShmemString>("ContextString");
    string sourceSentence(p1.first->begin(), p1.first->end());
    
    std::cout << "Received source string: " << sourceSentence << std::endl;
    
    PyObject* pyContextVectors = NULL;
    nmt.GetContextVectors(sourceSentence, pyContextVectors);
    
    std::pair<void**, size_t> p2 = segment.find<void*>("ContextPtr");
    *p2.first = pyContextVectors;
}

void HandleSentence(NMT_Wrapper &nmt,
                    bi::managed_shared_memory& segment) {
    
    std::pair<void**, size_t> p2 = segment.find<void*>("ContextPtr");
    std::pair<ShmemStringVector*, size_t > v1 = segment.find<ShmemStringVector>("AllWords");
    std::pair<ShmemStringVector*, size_t > v2 = segment.find<ShmemStringVector>("AllLastWords");
    std::pair<ShmemVoidptrVector*, size_t > v3 = segment.find<ShmemVoidptrVector>("AllStates");
    
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
    
    // batching
    nmt.GetNextLogProbStates(allWords, (PyObject*)*p2.first, allLastWords,
                             allStates, outProbs, allOutStates);
    
    std::pair<ShmemVoidptrVector*, size_t > v4 = segment.find<ShmemVoidptrVector>("AllOutStates");
    std::pair<ShmemFloatVector*, size_t > v5 = segment.find<ShmemFloatVector>("AllOutProbs");
    
    for(size_t i = 0; i < outProbs.size(); ++i) {
        v4.first->push_back(allOutStates[i]);
        v5.first->push_back(outProbs[i]);
    }
}

int main(int argc, char *argv[])
{
    std::string memName = "NeuralSharedMemory";
    if(argc == 2)
        memName = argv[1];
    bi::managed_shared_memory segment(bi::open_only, memName.c_str());
    
    std::string statePath, modelPath, wrapperPath, sourceVocab, targetVocab;
    NMT_Wrapper wrapper;
    
    bi::interprocess_mutex& mutex = GetMutex(segment);
    
    {
        bi::scoped_lock<bi::interprocess_mutex> lock(mutex);
        GetPaths(statePath, modelPath, wrapperPath, sourceVocab, targetVocab, segment);
        wrapper.Init(statePath, modelPath, wrapperPath, sourceVocab, targetVocab);
        NotifyParent(segment);
    }
    
    while(true) {
        bi::scoped_lock<bi::interprocess_mutex> lock(mutex);
        WaitForParent(segment, lock);

        State state = GetState(segment);
        std::cerr << "Got state " << state << std::endl;
        switch(state) {
            case EmptyHypothesis:
                HandleEmptyHypothesis(wrapper, segment);
                break;
            case InSentence:
                HandleSentence(wrapper, segment);
                break;
            case Exit:
                NotifyParent(segment);
                return 0;
        }
        
        NotifyParent(segment);
    }
    return 0;
}
