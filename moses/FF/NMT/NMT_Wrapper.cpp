#include "NMT_Wrapper.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <streambuf>

using namespace std;

NMT_Wrapper::NMT_Wrapper()
{
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


bool NMT_Wrapper::Init(const string& state_path, const string& model_path, const string& wrapper_path)
{

    this->state_path = state_path;
    this->model_path = model_path;

    Py_Initialize();

    AddPathToSys(wrapper_path);

    PyObject* filename = PyString_FromString((char*) "nmt_wrapper");
    PyObject* imp = PyImport_Import(filename);
    if (imp == NULL) {
        cerr << "No import\n"; return false;
    }

    PyObject* wrapper_name = PyObject_GetAttrString(imp, (char*)"NMTWrapper");
    if (wrapper_name == NULL) {
        cerr << "No wrapper\n"; return false;
    }

    PyObject* args = PyTuple_Pack(2, PyString_FromString(state_path.c_str()), PyString_FromString(model_path.c_str()));
    py_wrapper = PyObject_CallObject(wrapper_name, args);
    if (py_wrapper == NULL) {
        return false;
    }

    if (PyObject_CallMethod(py_wrapper, (char*)"build", NULL) == NULL) {
        return false;
    }

    py_get_log_prob = PyString_FromString((char*)"get_log_prob");
    py_get_log_probs = PyString_FromString((char*)"get_log_probs");
    py_get_context_vectors = PyString_FromString((char*)"get_context_vector");

    return true;
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

NMT_Wrapper::~NMT_Wrapper()
{
    Py_Finalize();
}

