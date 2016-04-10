#include "dsgHyp.h"
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <cstdlib>
#include <math.h>
#include <map>


using namespace std;
using namespace lm::ngram;

namespace Moses
{
dsgState::dsgState(const State & val)
{
  lmState = val;
}

void dsgState::saveState( std::vector<std::string> danglingTok, std::vector<int> srcSpans,float deltaValue)
{
  buffer = danglingTok;
  span=srcSpans;
  delta=deltaValue;
}


size_t dsgState::hash() const
{

  size_t ret = 0;
  boost::hash_combine(ret, lmState);

  /*size_t ret = delta;
  boost::hash_combine(ret, buffer);
  boost::hash_combine(ret, span);
  boost::hash_combine(ret, lmState.length);
  return ret;*/
}

bool dsgState::operator==(const FFState& otherBase) const   //CHECK
{
  const dsgState &other = static_cast<const dsgState&>(otherBase);

  if (lmState < other.lmState) return false;
  if (lmState == other.lmState) return true;
  return false;
}

// ----------------------------------------

std::string dsgState :: getName() const
{
  return "done";
}

dsgHypothesis :: dsgHypothesis()
{
  lmProb = 0;
  discontig0 = 0;
  discontig1 = 0;
  discontig2 = 0;
  UnsegWP = 0;
  m_buffer.clear();//="";
}

void dsgHypothesis :: setState(const FFState* prev_state)
{
  if(prev_state != NULL) {
    m_buffer = static_cast <const dsgState *> (prev_state)->getBuffer();
    m_span = static_cast <const dsgState *> (prev_state)->getSpan();
    lmState = static_cast <const dsgState *> (prev_state)->getLMState();
    delta = static_cast <const dsgState *> (prev_state)->getDelta(); //NEW
  }
}

dsgState * dsgHypothesis :: saveState()
{
  dsgState * statePtr = new dsgState(lmState);
  statePtr->saveState(m_buffer, m_span, delta);
  return statePtr;
}

void dsgHypothesis :: populateScores(vector <float> & scores , const int numFeatures)
{
  scores.clear();
  scores.push_back(lmProb);

  if (numFeatures == 1)
    return;
  scores.push_back(discontig0);
  scores.push_back(discontig1);
  scores.push_back(discontig2);
  scores.push_back(UnsegWP);
}



bool dsgHypothesis::isPrefix(const std::string &tok)
{
  if ((tok.at(tok.size() - 1) == '+' )&& (tok != "+")) {
    return true;
  } else  {
    return false;
  };
}

bool dsgHypothesis::isSuffix(const std::string &tok)
{
  if ((tok.at(0) == '+' )&& (tok != "+")) {
    return true;
  } else  {
    return false;
  };
}

bool dsgHypothesis::isStem(const std::string &tok)
{
  if ((tok.at(0) != '+') && (tok.at(tok.size() - 1) != '+')) {
    return true;
  } else  {
    return false;
  };
}



/**
 * chain stores segmented tokens that are in process of building a word
 * The function checks if tok contributes to the word being formed in chain
 *
 */
bool dsgHypothesis::isValidChain(const std::string &tok, std::vector<std::string> &chain)
{
  std::string last_tok;
  if (chain.size() >= 1) {
    last_tok = chain[chain.size() - 1];
  } else {
    last_tok = "NULL";
  }
  if(tok=="+") {
    return false;
  }
  if (isPrefix(tok) && (chain.size() == 0 || isPrefix(last_tok))) {
    return true;
  } else if (isSuffix(tok) && (chain.size() != 0 && ( isStem(last_tok) || isPrefix(last_tok)))) {
    return true;  // allows one suffix ONLY
  }
  //else if (isSuffix(tok) && (chain.size() != 0 && ( isStem(last_tok) || isPrefix(last_tok) || isSuffix(last_tok) ))) { return true; } // allows multiple suffixes
  else if (isStem(tok) && (chain.size() == 0 || isPrefix(last_tok))) {
    return true;
  } else {
    return false;
  }
}

/**
 * grouper function groups tokens that form a word together
 */
vector<string> dsgHypothesis::grouper(std::vector<std::string> &phr_vec,vector<vector<int> > &allchain_ids, int sourceOffset,const AlignmentInfo &align, bool isolation)
{

  std::vector<std::string> chain;
  std::vector<int> chain_ids;
  std::vector<std::string> allchains;
  chain_ids=m_span;

  if (!m_buffer.empty() && !isolation) { // if evaluate in isolation is called, then do not add buffer content
    for (int i = 0; i < m_buffer.size(); i++) {  // initialize chain with the content of the buffer
      chain.push_back(m_buffer[i]);
    }
  }

  for (int i = 0; i < phr_vec.size(); i++) {
    std::set<std::size_t> sourcePosSet = align.GetAlignmentsForTarget(i);

    if (isValidChain(phr_vec[i], chain)) {
      chain.push_back(phr_vec[i]);
      if (sourcePosSet.empty()==false) {
        for (std::set<size_t>::iterator it(sourcePosSet.begin()); it != sourcePosSet.end(); it++) {
          int cur=*it;
          chain_ids.push_back(cur+sourceOffset);
        }
      }
    }

    else if (chain.size() == 0) {  // start of a suffix at hypothesis0
      allchains.push_back(phr_vec[i]);
      allchain_ids.push_back(chain_ids);
      chain_ids.clear();//={};
    }

    else {  // tokens formed a complete word; add tokens segmented by space to allchains
      std::string joined = boost::algorithm::join(chain, " ");
      allchains.push_back(joined);
      allchain_ids.push_back(chain_ids);

      chain.clear();// = {};
      chain_ids.clear();//={};

      chain.push_back(phr_vec[i]);
      if (sourcePosSet.empty()==false) {
        for (std::set<size_t>::iterator it(sourcePosSet.begin()); it != sourcePosSet.end(); it++) {
          int cur=*it;
          chain_ids.push_back(cur+sourceOffset);
        }
      }

    }

  }

  if (!chain.empty()) {
    std::string joined = boost::algorithm::join(chain, " ");
    allchains.push_back(joined);
    allchain_ids.push_back(chain_ids);
  }
  return allchains;
}



void dsgHypothesis :: calculateDsgProbinIsol(DsgLM & ptrDsgLM, Desegmenter &desegT, const AlignmentInfo &align )
{
  lmProb = 0;
  State currState = lmState;
  State temp;
  string desegmented="";
  vector <string> words;
  vector <string> currFVec;

  discontig0=0;
  discontig1=0;
  discontig2=0;
  UnsegWP=0;

  currFVec = m_buffer;
  currFVec.insert( currFVec.end(), m_curr_phr.begin(), m_curr_phr.end() );

  int vecSize=currFVec.size();

  // phrases with suffix-starts and prefix-end
  if (currFVec.size()>0  && isPrefix (currFVec.back())) {
    UnsegWP-=0.5;
  }
  if (currFVec.size()>0  && isSuffix (currFVec.front())) {
    UnsegWP-=0.5;
  }

  /* //Dropping prefix-end and suffix-start
     while  (currFVec.size()>0 && isPrefix (currFVec.back())){
     currFVec.pop_back(); //drop prefix appearing at end of phrase
     }

     while (currFVec.size()>0 && isSuffix (currFVec.front())){
     currFVec.erase (currFVec.begin()); //drop suffix appearning at start of a phrase
     } */

  vector<vector<int> > chain_ids;
  words = grouper(currFVec,chain_ids,0,align,1);

  for (int i = 0; i<words.size(); i++) {
    UnsegWP+=1;
    temp = currState;
    if (words[i].find(" ")!=std::string::npos) {
      desegmented=desegT.Search(words[i])[0];
      lmProb += ptrDsgLM.Score(temp,desegmented,currState);
    } else {
      boost::replace_all(words[i], "-LRB-", "(");
      boost::replace_all(words[i], "-RRB-", ")");
      lmProb += ptrDsgLM.Score(temp,words[i],currState);
    }
  }
  lmState = currState;
}

void dsgHypothesis :: calculateDsgProb(DsgLM& ptrDsgLM, Desegmenter &desegT, bool isCompleted , const AlignmentInfo &align, int  sourceOffset, bool optimistic)
{
  lmProb = 0;
  discontig0=0;
  discontig1=0;
  discontig2=0;
  UnsegWP=0;

  State currState = lmState;
  State temp;
  string desegmented="";
  vector <string> words;
  vector <string> currFVec;
  bool completePhraseSuffixEnd = false;
  vector<vector<int> > all_chain_ids;
  double pscore;
  currFVec=m_curr_phr;

  // Check if the the phrase ends in a suffix, which means that it completes a full word;Make sure to change the isValidChain
  if (isSuffix (currFVec.back()) && (currFVec.back()!="+")) {
    completePhraseSuffixEnd=true;
  }

  words = grouper(currFVec,all_chain_ids,sourceOffset,align,0);

  for (int i = 0; i < words.size(); i++) {
    temp = currState;

    if (i==words.size()-1) {
      if (completePhraseSuffixEnd) {  //i.e if phrase ends with suffix, which marks an end of a word
        m_buffer.clear();// ="";
        m_span.clear();// ={};
      } else if (!isCompleted) { // not end of sentence( or final hypothesis), and probably the last token is not a complete word
        m_buffer.clear();
        if (optimistic == 1) {
          if ( isPrefix (currFVec.back())) { // this will delay scoring of prefix in prefix-ending phrases until the next hypothesis arrives
            //pscore = ptrDsgLM.Score(temp,desegmented,currState);
            lmProb -= delta;
            delta = 0.0;
          }

          else if (words[i].find(" ")!=std::string::npos) {
            desegmented=desegT.Search(words[i])[0];
            pscore=ptrDsgLM.Score(temp,desegmented,currState);
            lmProb = lmProb + pscore - delta;
            delta=pscore;
            currState=temp;
          } else {
            boost::replace_all(words[i], "-LRB-", "(");
            boost::replace_all(words[i], "-RRB-", ")");
            pscore=ptrDsgLM.Score(temp,words[i],currState);
            lmProb = lmProb + pscore - delta;
            delta=pscore;
            currState=temp;
          }
        }

        m_buffer.push_back(words.back());
        m_span=all_chain_ids.back();
        break;
      }
    }

    //temp = currState;
    if (words[i].find(" ")!=std::string::npos) {
      UnsegWP+=1;
      desegmented=desegT.Search(words[i])[0];
      std::set<int> cur_chain_ids(all_chain_ids[i].begin(),all_chain_ids[i].end());
      if (cur_chain_ids.size()>1) {
        vector<int> dsc;
        for (std::set<int>::iterator it(cur_chain_ids.begin()), next(it); it != cur_chain_ids.end() && ++next != cur_chain_ids.end(); it = next) {
          int cur=*it;
          int mynext=*next;
          if (std::abs(cur - mynext)>= 3) {
            dsc.push_back(3);
          } else if (std::abs(cur - mynext)== 2) {
            dsc.push_back(2);
          } else if (std::abs(cur - mynext)<= 1) {
            dsc.push_back(1);
          }
        }
        int mymax=*std::max_element(dsc.begin(),dsc.end());
        if (mymax==3) {
          discontig2+=1;
        } else if (mymax==2) {
          discontig1+=1;
        } else {
          discontig0+=1;
        }
      } else {
        discontig0 += 1;
      }

      lmProb += ptrDsgLM.Score(temp,desegmented,currState);
    } else {
      UnsegWP+=1;
      boost::replace_all(words[i], "-LRB-", "(");
      boost::replace_all(words[i], "-RRB-", ")");
      lmProb += ptrDsgLM.Score(temp,words[i],currState);
    }
  }

  if (isCompleted) {
    temp = currState;
    lmProb = lmProb + ptrDsgLM.ScoreEndSentence(temp,currState) - delta;
  }
  lmState = currState;
}


void dsgHypothesis :: print()
{}


} // namespace
