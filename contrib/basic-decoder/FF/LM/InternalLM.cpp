#include <vector>
#include <string>
#include <iostream>
#include "InternalLM.h"
#include "InputFileStream.h"
#include "Util.h"
#include "MyVocab.h"
#include "Phrase.h"

using namespace std;

namespace FastMoses
{

////////////////////////////////////////////////////////////////////////////////
InternalLMNode *InternalLMNode::GetOrCreateNode(VOCABID vocabId)
{
  Children::iterator iter;
  iter = m_children.find(vocabId);
  if (iter == m_children.end()) {
    return &m_children[vocabId];
  } else {
    InternalLMNode *node = &iter->second;
    return node;
  }
}

const InternalLMNode *InternalLMNode::Get(VOCABID vocabId) const
{
  Children::const_iterator iter;
  iter = m_children.find(vocabId);
  if (iter == m_children.end()) {
    return NULL;
  } else {
    const InternalLMNode *node = &iter->second;
    return node;
  }
}


//////////////////////////////////////////////////////////////////////////////
InternalLM::InternalLM(const std::string &line)
  :LM(line)
  ,m_lastNode(NULL)
{
  ReadParameters();
}

void InternalLM::Load()
{
  // 1st, set prob for root
  m_node.score = 0;
  m_node.logBackOff = 0;

  Moses::InputFileStream iniStrme(m_path);

  vector<string> toks;
  size_t lineNum = 0;
  string line;
  while (getline(iniStrme, line)) {
    lineNum++;
    if (lineNum % 1000000 == 0) {
      cerr << lineNum << " " << flush;
    }

    if (line.size() != 0 && line.substr(0,1) != "\\") {
      toks.clear();
      Tokenize(toks, line, "\t");

      if (toks.size() >= 2) {
        // split unigram/bigram trigrams
        vector<string> wordVec;
        Tokenize(wordVec, toks[1], " ");

        // create / traverse down tree
        InternalLMNode *node = &m_node;
        for (int pos = (int) wordVec.size() - 1 ; pos >= 0  ; pos--) {
          const string &wordStr = wordVec[pos];
          VOCABID vocabId = MyVocab::Instance().GetOrCreateId(wordStr);
          node = node->GetOrCreateNode(vocabId);
          assert(node);
        }
        assert(node);

        SCORE score = TransformSRIScore(Scan<SCORE>(toks[0]));
        node->score = score;
        if (toks.size() == 3) {
          SCORE logBackOff = TransformSRIScore(Scan<SCORE>(toks[2]));
          node->logBackOff = logBackOff;
        } else {
          node->logBackOff = 0;
        }
      }
    }
  }
}

size_t InternalLM::GetLastState() const
{
  assert(m_lastNode);
  size_t ret = (size_t) m_lastNode;
  return ret;
}

SCORE InternalLM::GetValue(const PhraseVec &phraseVec) const
{
  m_lastNode = &GetNode(phraseVec);
  assert(m_lastNode);
  return m_lastNode->score;
}

const InternalLMNode &InternalLM::GetNode(const PhraseVec &phraseVec) const
{
  size_t size = phraseVec.size();

  const InternalLMNode *node = &m_node;
  const InternalLMNode *prevNode = node;
  for (int pos = (int) size - 1 ; pos >= 0  ; pos--) {
    const Word &word = *phraseVec[pos];
    VOCABID vocabId = word.GetVocab();
    node = node->Get(vocabId);

    if (node) {
      prevNode = node;
    } else {
      node = prevNode;
      break;
    }
  }

  return *node;
}

void InternalLM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_path = value;
  } else {
    LM::SetParameter(key, value);
  }
}


}
