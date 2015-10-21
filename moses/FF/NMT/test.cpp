#include <iostream>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "NMT_Wrapper.h"

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

    boost::shared_ptr<NMT_Wrapper> wrapper(new NMT_Wrapper());
    wrapper->Init(statePath, modelPath, wrapperPath, sourceVocab, targetVocab);
    vector<double> prob;
    vector<PyObject*> nextStates;
    vector<string> nextWords;
    for (size_t i = 0; i < 1000; ++i) nextWords.push_back("das");

    vector<string> lastWords;
    for (size_t i = 0; i < 1000; ++i) lastWords.push_back("");
    vector<PyObject*> inputStates;
    for (size_t i = 0; i < 1000; ++i) inputStates.push_back(NULL);
    

    string sourceSentence = "this is a test sentence";
    PyObject* pyContextVectors = NULL;
    wrapper->GetContextVectors(sourceSentence, pyContextVectors);

    wrapper->GetNextLogProbStates(nextWords, pyContextVectors, lastWords,
                                  inputStates, prob, nextStates);
    cout << prob.size() << " " << prob[0] << endl;

    return 0;
}
