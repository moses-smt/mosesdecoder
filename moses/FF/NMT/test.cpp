#include "NMT_Wrapper.h"
#include <iostream>
#include <string>
#include <vector>
#include <boost/python.hpp>

using namespace std;
using namespace boost::python;

const int NCOPY = 10;

int main(int argc, char *argv[])
{
    string state_path = string(argv[1]);
    string model_path = string(argv[2]);
    string wrapper_path = string(argv[3]);

    NMT_Wrapper* wrapper = new NMT_Wrapper();
    bool res = wrapper->Init(state_path, model_path, wrapper_path);
    if (res == false) {
        cout << "No wrapper" << endl;
        return 1;
    }

    double prob;
    vector<PyObject*> nextStates;
    vector<PyObject*> currentStates;

    string source_sentence;
    string next_word;
    vector<string> lastWords;
    string last_word = "";
    source_sentence = "this is a test sentence";
    PyObject* py_context_vectors = NULL;
    wrapper->GetContextVectors(source_sentence, py_context_vectors);
    vector<string> nextWords;
    nextWords.push_back("das");
    nextWords.push_back("ich");
    vector<double> logProbs;
    wrapper->GetProb(nextWords,
                        py_context_vectors,
                        lastWords,
                        currentStates,
                        logProbs,
                        nextStates);
    for (size_t i = 0; i < logProbs.size(); ++i) {
        cout << "Word: " << nextWords[i] << "; Prob: " << logProbs[i] << endl;
    }
    cout << endl;
    currentStates = nextStates;
    lastWords = nextWords;
    nextWords.clear();
    nextWords.push_back("ist");
    nextWords.push_back("bin");
    wrapper->GetProb(nextWords,
                        py_context_vectors,
                        lastWords,
                        currentStates,
                        logProbs,
                        nextStates);
    for (size_t i = 0; i < logProbs.size(); ++i) {
        cout << "Word: " << nextWords[i] << "; Prob: " << logProbs[i] << endl;
    }
    cout << endl;
    currentStates = nextStates;
    lastWords = nextWords;
    nextWords.clear();
    nextWords.push_back("ein");
    nextWords.push_back("ein");
    wrapper->GetProb(nextWords,
                        py_context_vectors,
                        lastWords,
                        currentStates,
                        logProbs,
                        nextStates);
    for (size_t i = 0; i < logProbs.size(); ++i) {
        cout << "Word: " << nextWords[i] << "; Prob: " << logProbs[i] << endl;
    }
    cout << endl;
    currentStates = nextStates;
    lastWords = nextWords;
    nextWords.clear();
    nextWords.push_back("test");
    nextWords.push_back("q");
    wrapper->GetProb(nextWords,
                        py_context_vectors,
                        lastWords,
                        currentStates,
                        logProbs,
                        nextStates);
    for (size_t i = 0; i < logProbs.size(); ++i) {
        cout << "Word: " << nextWords[i] << "; Prob: " << logProbs[i] << endl;
    }

    return 0;
}
