#ifndef NMT_WRAPPER_H_
#define NMT_WRAPPER_H_

#include <string>
#include <vector>
#include <functional>
#include <python2.7/Python.h>


class NMT_Wrapper
{
public:
    explicit NMT_Wrapper();
    bool Init(const std::string& state_path,
              const std::string& model_path,
              const std::string& wrapper_path);

    bool GetContextVectors(const std::string& source_sentence, PyObject*& vectors);

    bool GetProb(const std::string& next_word,
                 PyObject* source_sentence,
                 const std::string& last_word,
                 PyObject* input_state,
                 double& prob,
                 PyObject*& output_state);
    bool GetProb(const std::vector<std::string>& nextWords,
                 PyObject* source_sentence,
                 const std::string& last_word,
                 PyObject* input_state,
                 double& prob,
                 PyObject*& output_state);
    bool GetProb(const std::vector<std::string>& nextWords,
                 PyObject* pyContextVectors,
                 const std::vector< std::string >& lastWords,
                 std::vector< PyObject* >& inputStates,
                 std::vector< std::vector< double > >& logProbs,
                 std::vector< std::vector< PyObject* > >& outputStates);
    virtual ~NMT_Wrapper();

private:
    PyObject* py_wrapper;
    PyObject* py_get_log_prob;
    PyObject* py_get_log_probs;
    PyObject* py_get_vec_log_probs;
    PyObject* py_get_context_vectors;
    std::string state_path;
    std::string model_path;
    void AddPathToSys(const std::string& path);
};


#endif  // NMT_WRAPPER_H_
