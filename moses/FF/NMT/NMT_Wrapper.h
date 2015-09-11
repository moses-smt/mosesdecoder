#ifndef NMT_WRAPPER_H_
#define NMT_WRAPPER_H_

#include <string>
#include <vector>
#include <functional>
#include <python2.7/Python.h>

using namespace std;

class NMT_Wrapper
{
public:
    explicit NMT_Wrapper();
    bool Init(const string& state_path,
              const string& model_path);

    bool GetContextVectors(const string& source_sentence, PyObject* vectors);

    bool GetProb(const string& next_word,
                 const string& source_sentence,
                 const string& last_word,
                 PyObject* input_state,
                 double& prob,
                 PyObject*& output_state);

    bool GetProb(const string& next_word,
                 PyObject* source_sentence,
                 const string& last_word,
                 PyObject* input_state,
                 double& prob,
                 PyObject* output_state);
    virtual ~NMT_Wrapper();

private:
   PyObject* py_wrapper;
   PyObject* py_get_prob;
   PyObject* py_get_context_vectors;
   string state_path;
   string model_path;
};


#endif  // NMT_WRAPPER_H_
