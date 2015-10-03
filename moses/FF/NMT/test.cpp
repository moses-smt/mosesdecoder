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

bool HandleEmptyHypothesis(NMT_Wrapper& nmt,
                           bi::managed_shared_memory& segment) {
    bi::scoped_lock<bi::interprocess_mutex> lock(GetMutex(segment));
    WaitForParent(segment, lock);
    std::pair<ShmemString*, size_t> p1 = segment.find<ShmemString>("ContextString");
    if(p1.second == 0)
        return false;
    string sourceSentence(p1.first->begin(), p1.first->end());
    
    std::cout << "Received source string: " << sourceSentence << std::endl;
    
    PyObject* pyContextVectors = NULL;
    nmt.GetContextVectors(sourceSentence, pyContextVectors);
    
    std::pair<void**, size_t> p2 = segment.find<void*>("ContextPtr");
    if(p2.second == 0)
        return false;
    *p2.first = pyContextVectors;
    
    NotifyParent(segment);
    return true;
}

void HandleSentence(NMT_Wrapper &nmt,
                    bi::managed_shared_memory& segment) {
    while(1) {
        bi::scoped_lock<bi::interprocess_mutex> lock(GetMutex(segment));
        WaitForParent(segment, lock);
    
        std::pair<bool*, size_t> p1 = segment.find<bool>("SentenceIsDone");
        if(*p1.first)
            break;
        
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
        
        NotifyParent(segment);
    }
    
    std::cerr << "Done with sentence" << std::endl;
}

int main(int argc, char *argv[])
{
    std::string memName = "NeuralSharedMemory";
    if(argc == 2)
        memName = argv[1];
    bi::managed_shared_memory segment(bi::open_only, memName.c_str());
    
    std::string statePath, modelPath, wrapperPath, sourceVocab, targetVocab;
    GetPaths(statePath, modelPath, wrapperPath, sourceVocab, targetVocab, segment);
    
    NMT_Wrapper wrapper;
    wrapper.Init(statePath, modelPath, wrapperPath, sourceVocab, targetVocab);
    NotifyParent(segment);
    
    while(HandleEmptyHypothesis(wrapper, segment))
        HandleSentence(wrapper, segment);
        
    return 0;
}
