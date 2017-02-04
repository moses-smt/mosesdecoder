#pragma once

#include <vector>
#include "Node.h"

namespace Moses2
{

template<class KeyClass, class ValueClass>
class InMemoryTrie
{
public:
  InMemoryTrie() {
  }
  Node<KeyClass, ValueClass>* insert(const std::vector<KeyClass>& word,
                                     const ValueClass& value);
  const Node<KeyClass, ValueClass>* getNode(
    const std::vector<KeyClass>& words) const;
  const Node<KeyClass, ValueClass> &getNode(const std::vector<KeyClass>& words,
      size_t &stoppedAtInd) const;
  std::vector<const Node<KeyClass, ValueClass>*> getNodes(
    const std::vector<KeyClass>& words, size_t &stoppedAtInd) const;
private:
  Node<KeyClass, ValueClass> root;
};

template<class KeyClass, class ValueClass>
Node<KeyClass, ValueClass>* InMemoryTrie<KeyClass, ValueClass>::insert(
  const std::vector<KeyClass>& word, const ValueClass& value)
{
  Node<KeyClass, ValueClass>* cNode = &root;
  for (size_t i = 0; i < word.size(); ++i) {
    KeyClass cKey = word[i];
    cNode = cNode->addSubnode(cKey);
  }
  cNode->setValue(value);
  return cNode;
}

template<class KeyClass, class ValueClass>
const Node<KeyClass, ValueClass>* InMemoryTrie<KeyClass, ValueClass>::getNode(
  const std::vector<KeyClass>& words) const
{
  size_t stoppedAtInd;
  const Node<KeyClass, ValueClass> &ret = getNode(words, stoppedAtInd);
  if (stoppedAtInd < words.size()) {
    return NULL;
  }
  return &ret;
}

template<class KeyClass, class ValueClass>
const Node<KeyClass, ValueClass> &InMemoryTrie<KeyClass, ValueClass>::getNode(
  const std::vector<KeyClass>& words, size_t &stoppedAtInd) const
{
  const Node<KeyClass, ValueClass> *prevNode = &root, *newNode;
  for (size_t i = 0; i < words.size(); ++i) {
    const KeyClass &cKey = words[i];
    newNode = prevNode->findSub(cKey);
    if (newNode == NULL) {
      stoppedAtInd = i;
      return *prevNode;
    }
    prevNode = newNode;
  }

  stoppedAtInd = words.size();
  return *newNode;
}

template<class KeyClass, class ValueClass>
std::vector<const Node<KeyClass, ValueClass>*> InMemoryTrie<KeyClass, ValueClass>::getNodes(
  const std::vector<KeyClass>& words, size_t &stoppedAtInd) const
{
  std::vector<const Node<KeyClass, ValueClass>*> ret;
  const Node<KeyClass, ValueClass> *prevNode = &root, *newNode;
  ret.push_back(prevNode);

  for (size_t i = 0; i < words.size(); ++i) {
    const KeyClass &cKey = words[i];
    newNode = prevNode->findSub(cKey);
    if (newNode == NULL) {
      stoppedAtInd = i;
      return ret;
    } else {
      ret.push_back(newNode);
    }
    prevNode = newNode;
  }

  stoppedAtInd = words.size();
  return ret;
}

}

