#ifndef NMT_WRAPPER_H_
#define NMT_WRAPPER_H_

#include <string>
#include <vector>
#include <set>
#include <functional>

struct _object;
typedef _object PyObject;

class NMT_Wrapper
{
public:
    NMT_Wrapper();
    ~NMT_Wrapper();

    void Init(
        const std::string& state_path,
        const std::string& model_path,
        const std::string& topn,
        const std::string& wrapper_path,
        const std::string& sourceVocabPath,
        const std::string& targetVocabPath);

    bool GetContextVectors(
                const std::string& source_sentence,
                PyObject*& vectors);

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
    void GetProb(const std::vector<std::string>& nextWords,
                 PyObject* pyContextVectors,
                 const std::vector< std::string >& lastWords,
                 std::vector<PyObject*>& inputStates,
                 std::vector< std::vector< double > >& logProbs,
                 std::vector< std::vector< PyObject* > >& outputStates);
    void GetProb(const std::vector<std::string>& nextWords,
                 PyObject* pyContextVectors,
                 const std::vector< std::string >& lastWords,
                 std::vector<PyObject*>& inputStates,
                 std::vector< std::vector< double > >& logProbs,
                 std::vector< std::vector< PyObject* > >& outputStates,
                 std::vector<bool>& unks);
    void GetNextStates(
        const std::vector<std::string>& nextWords,
        PyObject* pyContextVectors,
        std::vector<PyObject*>& inputStates,
        std::vector<PyObject*>& outputStates);

    void GetNextLogProbStates(
        const std::vector<std::string>& nextWords,
        PyObject* pyContextVectors,
        const std::vector< std::string >& lastWords,
        std::vector<PyObject*>& inputStates,
        std::vector<double>& logProbs,
        std::vector<PyObject*>& nextStates);

    void GetNextLogProbStates(
        const std::vector<std::string>& nextWords,
        PyObject* pyContextVectors,
        const std::vector< std::string >& lastWords,
        std::vector<PyObject*>& inputStates,
        std::vector<double>& logProbs,
        std::vector<PyObject*>& nextStates,
        std::vector<bool>& unks);

    static NMT_Wrapper& GetNMT() {
        return *s_nmt;
    }

    static void SetNMT(NMT_Wrapper* ptr) {
        s_nmt = ptr;
    }

    std::vector<bool> IsUnk(const std::vector<std::string>& words);

private:
    static NMT_Wrapper* s_nmt;

    PyObject* py_wrapper;
    PyObject* py_get_log_prob;
    PyObject* py_get_log_probs;
    PyObject* py_get_vec_log_probs;
    PyObject* py_get_context_vectors;
    PyObject* py_get_next_states;
    PyObject* py_get_log_prob_states;
    std::set<std::string> m_targetVocab;
    void AddPathToSys(const std::string& path);
    void LoadTargetVocab();
};

#endif  // NMT_WRAPPER_H_
