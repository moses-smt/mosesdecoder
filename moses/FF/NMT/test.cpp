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
    wrapper->Init(state_path, model_path, wrapper_path, "", "");

    vector< vector<PyObject*> > nextStates;
    vector<PyObject*> currentStates;
    vector< vector<double> > logProbs;
    vector<string> nextWords;
    vector<string> lastWords;

    string source_sentence;
    string next_word;
    string last_word = "";
    source_sentence = "this is a test sentence";
    PyObject* py_context_vectors = NULL;
    wrapper->GetContextVectors(source_sentence, py_context_vectors);
    nextWords.push_back("das");
    nextWords.push_back("ich");
    nextWords.push_back("ich");
    wrapper->GetProb(nextWords,
                        py_context_vectors,
                        lastWords,
                        currentStates,
                        logProbs,
                        nextStates);
    cout << "OK" << endl;
    for (size_t i = 0; i < logProbs.size(); ++i) {
        cout << "hip: " << i << endl;
        for (size_t j = 0; j < logProbs[0].size(); ++j) {
            cout << "Word: " << nextWords[j] << "; Prob: " << logProbs[i][j] << endl;
        }
    }
    currentStates.clear();
    currentStates.push_back(nextStates[0][0]);
    currentStates.push_back(nextStates[0][1]);
    cout << "DONE" <<endl;

    lastWords.clear();
    lastWords.push_back(nextWords[0]);
    lastWords.push_back(nextWords[1]);
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
        cout << "hip: " << i << endl;
        for (size_t j =0; j < logProbs[0].size(); ++j){
            cout << "Word: " << nextWords[j] << "; Prob: " << logProbs[i][j] << endl;
        }
    }
    cout << endl;
    currentStates.clear();
    currentStates.push_back(nextStates[0][0]);
    currentStates.push_back(nextStates[1][1]);

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
        cout << "hip: " << i << endl;
        for (size_t j =0; j < logProbs[0].size(); ++j){
            cout << "Word: " << nextWords[j] << "; Prob: " << logProbs[i][j] << endl;
        }
    }
    cout << endl;
    currentStates.clear();
    currentStates.push_back(nextStates[0][0]);
    currentStates.push_back(nextStates[1][1]);

    lastWords = nextWords;
    nextWords.clear();
    nextWords.push_back("test");
    wrapper->GetProb(nextWords,
                        py_context_vectors,
                        lastWords,
                        currentStates,
                        logProbs,
                        nextStates);

    for (size_t i = 0; i < logProbs.size(); ++i) {
        cout << "hip: " << i << endl;
        for (size_t j =0; j < logProbs[0].size(); ++j){
            cout << "Word: " << nextWords[j] << "; Prob: " << logProbs[i][j] << endl;
        }
    }
    cout << endl;

    return 0;
}
