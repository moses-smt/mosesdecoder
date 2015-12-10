#include <string>
#include <map>
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"
#include "moses/FF/InternalTree.h"
#include "moses/Word.h"

#include <boost/thread/tss.hpp>
#include <boost/array.hpp>

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#endif


// relational dependency language model, described in:
// Sennrich, Rico (2015). Modelling and Optimizing on Syntactic N-Grams for Statistical Machine Translation. Transactions of the Association for Computational Linguistics.
// see 'scripts/training/rdlm' for training scripts

namespace nplm
{
class neuralTM;
}

namespace Moses
{

namespace rdlm
{

// we re-use some short-lived objects to reduce the number of allocations;
// each thread gets its own instance to prevent collision
// [could be replaced with thread_local keyword in C++11]
class ThreadLocal
{
public:
  std::vector<int> ancestor_heads;
  std::vector<int> ancestor_labels;
  std::vector<int> ngram;
  std::vector<int> heads;
  std::vector<int> labels;
  std::vector<int> heads_output;
  std::vector<int> labels_output;
  std::vector<std::pair<InternalTree*,std::vector<TreePointer>::const_iterator> > stack;
  nplm::neuralTM* lm_head;
  nplm::neuralTM* lm_label;

  ThreadLocal(nplm::neuralTM *lm_head_base_instance_, nplm::neuralTM *lm_label_base_instance_, bool normalizeHeadLM, bool normalizeLabelLM, int cacheSize);
  ~ThreadLocal();
};
}

class RDLMState : public TreeState
{
  float m_approx_head; //score that was approximated due to lack of context
  float m_approx_label;
  size_t m_hash;
public:
  RDLMState(TreePointer tree, float approx_head, float approx_label, size_t hash)
    : TreeState(tree)
    , m_approx_head(approx_head)
    , m_approx_label(approx_label)
    , m_hash(hash)
  {}

  float GetApproximateScoreHead() const {
    return m_approx_head;
  }

  float GetApproximateScoreLabel() const {
    return m_approx_label;
  }

  size_t GetHash() const {
    return m_hash;
  }

  int Compare(const FFState& other) const {
    if (m_hash == static_cast<const RDLMState*>(&other)->GetHash()) return 0;
    else if (m_hash > static_cast<const RDLMState*>(&other)->GetHash()) return 1;
    else return -1;
  }
};

class RDLM : public StatefulFeatureFunction
{
  typedef std::map<InternalTree*,TreePointer> TreePointerMap;

  nplm::neuralTM* lm_head_base_instance_;
  nplm::neuralTM* lm_label_base_instance_;

  mutable boost::thread_specific_ptr<rdlm::ThreadLocal> thread_objects_backend_;

  std::string m_glueSymbolString;
  Word dummy_head;
  Word m_glueSymbol;
  Word m_startSymbol;
  Word m_endSymbol;
  Word m_endTag;
  std::string m_path_head_lm;
  std::string m_path_label_lm;
  bool m_isPretermBackoff;
  size_t m_context_left;
  size_t m_context_right;
  size_t m_context_up;
  bool m_premultiply;
  bool m_rerank;
  bool m_normalizeHeadLM;
  bool m_normalizeLabelLM;
  bool m_sharedVocab;
  std::string m_debugPath; // score all trees in the provided file, then exit
  int m_binarized;
  int m_cacheSize;

  size_t offset_up_head;
  size_t offset_up_label;

  size_t size_head;
  size_t size_label;
  std::vector<int> static_label_null;
  std::vector<int> static_head_null;
  int static_dummy_head;
  int static_start_head;
  int static_start_label;
  int static_stop_head;
  int static_stop_label;
  int static_head_head;
  int static_head_label;
  int static_root_head;
  int static_root_label;

  int static_head_label_output;
  int static_stop_label_output;
  int static_start_label_output;

  FactorType m_factorType;

  static const int LABEL_INPUT = 0;
  static const int LABEL_OUTPUT = 1;
  static const int HEAD_INPUT = 2;
  static const int HEAD_OUTPUT = 3;
  mutable std::vector<int> factor2id_label_input;
  mutable std::vector<int> factor2id_label_output;
  mutable std::vector<int> factor2id_head_input;
  mutable std::vector<int> factor2id_head_output;

#ifdef WITH_THREADS
  //reader-writer lock
  mutable boost::shared_mutex m_accessLock;
#endif

public:
  RDLM(const std::string &line)
    : StatefulFeatureFunction(2, line)
    , m_glueSymbolString("Q")
    , m_isPretermBackoff(true)
    , m_context_left(3)
    , m_context_right(0)
    , m_context_up(2)
    , m_premultiply(true)
    , m_rerank(false)
    , m_normalizeHeadLM(false)
    , m_normalizeLabelLM(false)
    , m_sharedVocab(false)
    , m_binarized(0)
    , m_cacheSize(1000000)
    , m_factorType(0) {
    ReadParameters();
    std::vector<FactorType> factors;
    factors.push_back(0);
    dummy_head.CreateFromString(Output, factors, "<dummy_head>", false);
    m_glueSymbol.CreateFromString(Output, factors, m_glueSymbolString, true);
    m_startSymbol.CreateFromString(Output, factors, "SSTART", true);
    m_endSymbol.CreateFromString(Output, factors, "SEND", true);
    m_endTag.CreateFromString(Output, factors, "</s>", false);
  }

  ~RDLM();

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new RDLMState(TreePointer(), 0, 0, 0);
  }

  void Score(InternalTree* root, const TreePointerMap & back_pointers, boost::array<float,4> &score, size_t &boundary_hash, rdlm::ThreadLocal &thread_objects, int num_virtual = 0, int rescoring_levels = 0) const;
  bool GetHead(InternalTree* root, const TreePointerMap & back_pointers, std::pair<int,int> & IDs) const;
  void GetChildHeadsAndLabels(InternalTree *root, const TreePointerMap & back_pointers, int reached_end, rdlm::ThreadLocal &thread_objects) const;
  void GetIDs(const Word & head, const Word & preterminal, std::pair<int,int> & IDs) const;
  int Factor2ID(const Factor * const factor, int model_type) const;
  void ScoreFile(std::string &path); //for debugging
  void PrintInfo(std::vector<int> &ngram, nplm::neuralTM* lm) const; //for debugging

  TreePointerMap AssociateLeafNTs(InternalTree* root, const std::vector<TreePointer> &previous) const;

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void SetParameter(const std::string& key, const std::string& value);

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const {
    UTIL_THROW(util::Exception, "Not implemented");
  };
  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void Load(AllOptions::ptr const& opts);

  // Iterator-class that yields all children of a node; if child is virtual node of binarized tree, its children are yielded instead.
  class UnbinarizedChildren
  {
  private:
    std::vector<TreePointer>::const_iterator iter;
    std::vector<TreePointer>::const_iterator _begin;
    bool _ended;
    InternalTree* current;
    const TreePointerMap & back_pointers;
    bool binarized;
    std::vector<std::pair<InternalTree*,std::vector<TreePointer>::const_iterator> > &stack;

  public:
    UnbinarizedChildren(InternalTree* root, const TreePointerMap & pointers, bool binary, std::vector<std::pair<InternalTree*,std::vector<TreePointer>::const_iterator> > & persistent_stack):
      current(root),
      back_pointers(pointers),
      binarized(binary),
      stack(persistent_stack) {
      stack.resize(0);
      _ended = current->GetChildren().empty();
      iter = current->GetChildren().begin();
      // expand virtual node
      while (binarized && !(*iter)->GetLabel().GetString(0).empty() && (*iter)->GetLabel().GetString(0).data()[0] == '^') {
        stack.push_back(std::make_pair(current, iter));
        // also go through trees or previous hypotheses to rescore nodes for which more context has become available
        if ((*iter)->IsLeafNT()) {
          current = back_pointers.find(iter->get())->second.get();
        } else {
          current = iter->get();
        }
        iter = current->GetChildren().begin();
      }
      _begin = iter;
    }

    std::vector<TreePointer>::const_iterator begin() const {
      return _begin;
    }
    bool ended() const {
      return _ended;
    }

    std::vector<TreePointer>::const_iterator operator++() {
      iter++;
      if (iter == current->GetChildren().end()) {
        while (!stack.empty()) {
          std::pair<InternalTree*,std::vector<TreePointer>::const_iterator> & active = stack.back();
          current = active.first;
          iter = ++active.second;
          stack.pop_back();
          if (iter != current->GetChildren().end()) {
            break;
          }
        }
        if (iter == current->GetChildren().end()) {
          _ended = true;
          return iter;
        }
      }
      // expand virtual node
      while (binarized && !(*iter)->GetLabel().GetString(0).empty() && (*iter)->GetLabel().GetString(0).data()[0] == '^') {
        stack.push_back(std::make_pair(current, iter));
        // also go through trees or previous hypotheses to rescore nodes for which more context has become available
        if ((*iter)->IsLeafNT()) {
          current = back_pointers.find(iter->get())->second.get();
        } else {
          current = iter->get();
        }
        iter = current->GetChildren().begin();
      }
      return iter;
    }
  };

};

}
