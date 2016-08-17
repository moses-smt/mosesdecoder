#pragma once


# include "moses/FF/FFState.h"
# include "moses/Manager.h"
# include <set>
# include <map>
# include <string>
# include <vector>
# include "moses/FF/Dsg-Feature/Desegmenter.h"
# include "KenDsg.h"


namespace Moses
{

class dsgState : public FFState
{
public:

  dsgState(const lm::ngram::State & val);
  virtual bool operator==(const FFState& other) const;
  void saveState( std::vector<std::string>  bufferVal,std::vector<int> spanVal, float deltaValue);

  std::vector<std::string> getBuffer() const {
    return buffer;
  }

  std::vector<int> getSpan() const {
    return span;
  }

  lm::ngram::State getLMState() const {
    return lmState;
  }

  float getDelta() const {
    return delta;
  }

  void setDelta(double val1 ) {
    delta = val1;
  }

  void print() const;
  std::string getName() const;

  virtual size_t hash() const;


protected:
  std::vector<std::string> buffer;
  std::vector<int> span;
  lm::ngram::State lmState;
  double delta; //NEW
};



class dsgHypothesis
{

private:
  std::vector<std::string> m_buffer;// maintains dangling affix from previous hypothesis
  std::vector<int> m_span;// maintains source alignment for dangling affix from previous hypothesis
  lm::ngram::State lmState; // KenLM's Model State ...
  std::vector<std::string> m_curr_phr; //phrase from current hypothesis
  double delta; //NEW

  double lmProb;
  int discontig0;
  int discontig1;
  int discontig2;
  double UnsegWP; //Word Penalty score based on count of words

public:

  dsgHypothesis();
  ~dsgHypothesis() {};
  void calculateDsgProb(DsgLM& ptrDsgLM, Desegmenter &, bool isCompleted, const AlignmentInfo &align, int  sourceOffset, bool optimistic);
  void calculateDsgProbinIsol(DsgLM& ptrDsgLM, Desegmenter &, const AlignmentInfo &align);

  void setPhrases(std::vector<std::string> & val1 ) {
    m_curr_phr = val1;
  }

  void setDelta(double val1 ) {
    delta = val1;
  }

  void setState(const FFState* prev_state);
  dsgState * saveState();
  void print();
  void populateScores(std::vector <float> & scores , const int numFeatures);
  void setState(const lm::ngram::State & val) {
    lmState = val;
  }

  bool isPrefix(const std::string &);
  bool isSuffix(const std::string &);
  bool isStem(const std::string &);
  bool isValidChain(const  std::string  &, std::vector<std::string> &chain);
  vector<string> grouper(std::vector<std::string> &,std::vector<std::vector<int> > &,int,const AlignmentInfo &align,bool);

};
} // namespace


