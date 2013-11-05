#pragma once
#include <string>
#include <boost/unordered_map.hpp>
#include "LM.h"

namespace FastMoses
{

class InternalLMNode
{
public:
  typedef boost::unordered_map<VOCABID, InternalLMNode> Children;

  InternalLMNode *GetOrCreateNode(VOCABID vocabId);
  const InternalLMNode *Get(VOCABID vocabId) const;

  SCORE score, logBackOff;
protected:
  Children m_children;
};

class InternalLM : public LM
{
public:
  InternalLM(const std::string &line);
  void Load();
  virtual size_t GetLastState() const;

  void SetParameter(const std::string& key, const std::string& value);

protected:
  InternalLMNode m_node;
  std::string m_path;
  const InternalLMNode &GetNode(const PhraseVec &phraseVec) const;

  mutable const InternalLMNode *m_lastNode;

  virtual SCORE GetValue(const PhraseVec &phraseVec) const;
};

}
