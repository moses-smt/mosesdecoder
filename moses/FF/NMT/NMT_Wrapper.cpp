#include "NMT_Wrapper.h"

#include "util/exception.hh"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <streambuf>
#include <assert.h>

#include <python2.7/Python.h>

using namespace std;
using namespace util;

NMT_Wrapper* NMT_Wrapper::s_nmt;

NMT_Wrapper::NMT_Wrapper()
{
    py_get_log_prob = PyString_FromString((char*)"get_log_prob");
    py_get_log_probs = PyString_FromString((char*)"get_log_probs");
    py_get_vec_log_probs = PyString_FromString((char*)"get_vec_log_probs");
    py_get_context_vectors = PyString_FromString((char*)"get_context_vector");
    py_get_next_states = PyString_FromString((char*)"get_next_states");
    py_get_log_prob_states = PyString_FromString((char*)"get_log_prob_states");

    SetNMT(this);
}

NMT_Wrapper::~NMT_Wrapper () {
    Py_Finalize();
}

void NMT_Wrapper::LoadTargetVocab()
{
    PyObject* py_response = PyObject_CallMethodObjArgs(
                                py_wrapper,
                                PyString_FromString("get_target_vocab"),
                                NULL);
    const size_t vocabSize = PyList_Size(py_response);
    for (size_t i = 0; i < vocabSize; ++i) {
        m_targetVocab.insert(PyString_AsString(PyList_GetItem(py_response, i)));
    }
}

std::vector<bool> NMT_Wrapper::IsUnk(const std::vector<std::string>& words)
{
    std::vector<bool> isUnk(words.size());
    for (size_t i = 0; i < words.size(); ++i) {
        if(m_targetVocab.find(words[i]) != m_targetVocab.end()) {
            isUnk[i] = true;
        } else {
            isUnk[i] = false;
        }
    }
    return isUnk;
}

bool NMT_Wrapper::GetContextVectors(const string& source_sentence, PyObject*& vectors)
{
    PyObject* py_source_sentence = PyString_FromString(source_sentence.c_str());
    vectors = PyObject_CallMethodObjArgs(py_wrapper, py_get_context_vectors, py_source_sentence, NULL);
    return true;
}

void NMT_Wrapper::AddPathToSys(const string& path)
{
    PyObject* py_sys_path = PySys_GetObject((char*)"path");
    PyList_Append(py_sys_path, PyString_FromString(path.c_str()));
}

void NMT_Wrapper::Init(
        const std::string& state_path,
        const std::string& model_path,
        const std::string& wrapper_path,
        const std::string& sourceVocabPath,
        const std::string& targetVocabPath)
{
    Py_Initialize();
    AddPathToSys(wrapper_path);

    PyObject* filename = PyString_FromString((char*) "nmt_wrapper");
    PyObject* imp = PyImport_Import(filename);
    UTIL_THROW_IF2(imp == NULL, "The wrapper module could not be imported.");

    PyObject* wrapper_name = PyObject_GetAttrString(imp, (char*)"NMTWrapper");
    UTIL_THROW_IF2(wrapper_name == NULL, "It could not find NMTWrapper class.");

    PyObject* args = PyTuple_Pack(4,
            PyString_FromString(state_path.c_str()),
            PyString_FromString(model_path.c_str()),
            PyString_FromString(sourceVocabPath.c_str()),
            PyString_FromString(targetVocabPath.c_str()));

    py_wrapper = PyObject_CallObject(wrapper_name, args);
    UTIL_THROW_IF2(py_wrapper == NULL, "Problem with creating NMT_Wrapper.");

    UTIL_THROW_IF2(PyObject_CallMethod(py_wrapper, (char*)"build", NULL) == NULL,
            "Problem with build NMT_Wrapper");

    LoadTargetVocab();
}

bool NMT_Wrapper::GetProb(const string& next_word,
                          PyObject* py_context_vectors,
                          const string& last_word,
                          PyObject* input_state,
                          double& output_prob,
                          PyObject*& output_state)
{
    PyObject* py_next_word = PyString_FromString(next_word.c_str());
    PyObject* py_response = NULL;

    if (input_state == NULL)
    {
        py_response = PyObject_CallMethodObjArgs(py_wrapper, py_get_log_prob, py_next_word, py_context_vectors, NULL);
    }
    else {
        PyObject* py_last_word = PyString_FromString(last_word.c_str());
        py_response = PyObject_CallMethodObjArgs(py_wrapper, py_get_log_prob, py_next_word, py_context_vectors,
                                                 py_last_word, input_state, NULL);
    }

   if (py_response == NULL) { return false; }
    if (! PyTuple_Check(py_response)) { return false; }

    PyObject* py_prob = PyTuple_GetItem(py_response, 0);
    if (py_prob == NULL) { return false; }
    output_prob = PyFloat_AsDouble(py_prob);

    output_state = PyTuple_GetItem(py_response, 1);
    if (output_state == NULL) { return 0; }

    return true;
}

bool NMT_Wrapper::GetProb(const std::vector<std::string>& next_words,
                          PyObject* py_context_vectors,
                          const string& last_word,
                          PyObject* input_state,
                          double& logProb,
                          PyObject*& output_state)
{
    PyObject* py_nextWords = PyList_New(0);
    for (size_t i = 0; i < next_words.size(); ++i) {
        PyList_Append(py_nextWords, PyString_FromString(next_words[i].c_str()));
    }

    PyObject* py_response = NULL;

    if (input_state == NULL)
    {
        py_response = PyObject_CallMethodObjArgs(py_wrapper, py_get_log_probs,
                                                 py_nextWords, py_context_vectors, NULL);
    }
    else {
        PyObject* py_last_word = PyString_FromString(last_word.c_str());
        py_response = PyObject_CallMethodObjArgs(py_wrapper, py_get_log_probs, py_nextWords, py_context_vectors,
                                                 py_last_word, input_state, NULL);
    }

    if (py_response == NULL) { return false; }
    if (! PyTuple_Check(py_response)) { return false; }

    PyObject* py_prob = PyTuple_GetItem(py_response, 0);
    if (py_prob == NULL) { return false; }
    logProb = PyFloat_AsDouble(py_prob);

    output_state = PyTuple_GetItem(py_response, 1);
    if (output_state == NULL) { return 0; }

    return true;
}

bool NMT_Wrapper::GetProb(const std::vector<std::string>& nextWords,
                          PyObject* pyContextVectors,
                          const std::vector< string >& lastWords,
                          std::vector< PyObject* >& inputStates,
                          std::vector< std::vector< double > >& logProbs,
                          std::vector< std::vector< PyObject* > >& outputStates,
{
    std::vector<bool> unks;
    GetProb(nextWords,
            pyContextVectors,
            lastWords,
            inputStates,
            logProbs,
            outputStates,
            unks);
}

bool NMT_Wrapper::GetProb(const std::vector<std::string>& nextWords,
                          PyObject* pyContextVectors,
                          const std::vector< string >& lastWords,
                          std::vector< PyObject* >& inputStates,
                          std::vector< std::vector< double > >& logProbs,
                          std::vector< std::vector< PyObject* > >& outputStates,
                          std::vector<bool> unks)
{
    PyObject* pyNextWords = PyList_New(0);
    for (size_t i = 0; i < nextWords.size(); ++i) {
        PyList_Append(pyNextWords, PyString_FromString(nextWords[i].c_str()));
    }

    PyObject* pyLastWords = PyList_New(0);
    for (size_t i = 0; i < lastWords.size(); ++i) {
        PyList_Append(pyLastWords, PyString_FromString(lastWords[i].c_str()));
    }

    PyObject* pyResponse = NULL;
    if (inputStates.size() == 0) {
        pyResponse = PyObject_CallMethodObjArgs(py_wrapper,
                                                py_get_vec_log_probs,
                                                pyNextWords,
                                                pyContextVectors,
                                                NULL);
    } else {
        PyObject* pyInputStates = PyList_New(0);
        for (size_t i = 0; i < inputStates.size(); ++i) {
            PyList_Append(pyInputStates, inputStates[i]);
        }

        PyObject* pyLastWords = PyList_New(0);
        for (size_t i = 0; i < lastWords.size(); ++i) {
            PyList_Append(pyLastWords, PyString_FromString(lastWords[i].c_str()));
        }

        pyResponse = PyObject_CallMethodObjArgs(py_wrapper,
                                                py_get_vec_log_probs,
                                                pyNextWords,
                                                pyContextVectors,
                                                pyLastWords,
                                                pyInputStates,
                                                NULL);
    }
    UTIL_THROW_IF2(pyResponse == NULL, "No response from python module.");

    size_t inputSize = 0;
    if (inputStates.size() == 0) {
        inputSize = 1;
    } else {
        inputSize = inputStates.size();
    }

    PyObject* pyLogProbMatrix = PyTuple_GetItem(pyResponse, 0);
    PyObject* pyOutputStateMatrix = PyTuple_GetItem(pyResponse, 1);
    logProbs.clear();
    outputStates = vector<vector<PyObject*> >(inputSize, vector<PyObject*>(nextWords.size(), NULL));
    vector<double> hipoProbs;
    vector<PyObject*> hipoStates;
    logProbs.clear();
    for (size_t i = 0; i < inputSize; ++i) {
        hipoProbs.clear();
        hipoStates.clear();

        PyObject* pyLogProbColumn = PyList_GetItem(pyLogProbMatrix, i);
        for (size_t j = 0; j < nextWords.size(); ++j) {
            hipoProbs.push_back(PyFloat_AsDouble(PyList_GetItem(pyLogProbColumn, j)));
        }
        logProbs.push_back(hipoProbs);
    }

    for (size_t j = 0; j < nextWords.size(); ++j) {
        PyObject* pyOutputStateColumn = PyList_GetItem(pyOutputStateMatrix, j);
        for (size_t i = 0; i < inputSize; ++i) {
            outputStates[i][j]  = PyList_GetItem(pyOutputStateColumn, i);
        }

        outputStates.push_back(hipoStates);
    }
    unks = IsUnk(nextWords);

    return true;
}
void NMT_Wrapper::GetNextStates(
        const std::vector<std::string>& nextWords,
        PyObject* pyContextVectors,
        std::vector<PyObject*>& inputStates,
        std::vector<PyObject*>& nextStates)
{
    UTIL_THROW_IF2(nextWords.size() != nextStates.size(), "#nextWords != #inputStates");
    UTIL_THROW_IF2(nextWords.size() == 0, "No hipothesis!");

    PyObject* pyNextWords = PyList_New(0);
    for (size_t i = 0; i < nextWords.size(); ++i) {
        PyList_Append(pyNextWords, PyString_FromString(nextWords[i].c_str()));
    }

    PyObject* pyInputStates = PyList_New(0);
    for (size_t i = 0; i < inputStates.size(); ++i) {
        PyList_Append(pyInputStates, inputStates[i]);
    }

    PyObject* pyResponse = PyObject_CallMethodObjArgs(py_wrapper,
                                            py_get_next_states,
                                            pyNextWords,
                                            pyContextVectors,
                                            pyInputStates,
                                            NULL);

    UTIL_THROW_IF2(pyResponse == NULL, "No response from python module.");

    nextStates.clear();
    size_t nextStatesSize = PyList_Size(pyResponse);
    UTIL_THROW_IF2(nextStatesSize != inputStates.size(), "Returned bad number of states.");
    for (size_t i = 0; i < nextStatesSize; ++i) {
        nextStates.push_back(PyList_GetItem(pyResponse, i));
    }
}

void NMT_Wrapper::GetNextLogProbStates(
    const std::vector<std::string>& nextWords,
    PyObject* pyContextVectors,
    const std::vector< std::string >& lastWords,
    std::vector<PyObject*>& inputStates,
    std::vector<double>& logProbs,
    std::vector<PyObject*>& nextStates)
{
    std::vector<bool> unks;
    GetNextLogProbStates(nextWords,
                         pyContextVectors,
                         lastWords,
                         inputStates,
                         logProbs,
                         nextWords,
                         unks);
}

inline PyObject* StringVector2Python(std::vector<std::string> inputVector)
{
    PyObject* pyList = PyList_New(0);
    for (size_t i = 0; i < nextWords.size(); ++i) {
        PyList_Append(pyList, PyString_FromString(inputVector[i].c_str()));
    }
    return pyList;
}

void NMT_Wrapper::GetNextLogProbStates(
    const std::vector<std::string>& nextWords,
    PyObject* pyContextVectors,
    const std::vector< std::string >& lastWords,
    std::vector<PyObject*>& inputStates,
    std::vector<double>& logProbs,
    std::vector<PyObject*>& nextStates,
    std::vector<bool>& unks)
{
    UTIL_THROW_IF2(lastWords.size() != inputStates.size(), "#lastWords != #inputStates");

    PyObject* pyNextWords = StringVector2Python(nextWords);
    PyObject* pyLastWords = StringVector2Python(lastWords);

    PyObject* pyStates = PyList_New(0);
    for (size_t i = 0; i < inputStates.size(); ++i) {
        PyList_Append(pyStates, inputStates[i]);
    }
    PyObject* pyResponse = NULL;

    if (inputStates.size() == 0) {
        pyResponse = PyObject_CallMethodObjArgs(py_wrapper,
                                                py_get_log_prob_states,
                                                pyNextWords,
                                                pyContextVectors,
                                                NULL);
    } else {
        pyResponse = PyObject_CallMethodObjArgs(py_wrapper,
                                                py_get_log_prob_states,
                                                pyNextWords,
                                                pyContextVectors,
                                                pyLastWords,
                                                pyStates,
                                                NULL);
    }
    UTIL_THROW_IF2(pyResponse == NULL, "No response from python module.");

    size_t inputSize = 0;
    if (inputStates.size() == 0) {
        inputSize = 1;
    } else {
        inputSize = inputStates.size();
    }

    PyObject* pyLogProbs = PyTuple_GetItem(pyResponse, 0);
    PyObject* pyNextStates = PyTuple_GetItem(pyResponse, 1);
    logProbs.clear();
    nextStates.clear();

    for (size_t j = 0; j < inputSize; ++j) {
        logProbs.push_back(PyFloat_AsDouble(PyList_GetItem(pyLogProbs, j)));
    }
    for (size_t i = 0; i < inputSize; ++i) {
        PyObject* nextState = PyList_GetItem(pyNextStates, i);
        if (nextState == NULL) cerr << "NULL OUTOUT" << endl;
        nextStates.push_back(nextState);
    }
    unks = IsUnk(nextWords);
}
