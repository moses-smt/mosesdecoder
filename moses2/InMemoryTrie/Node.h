#pragma once

#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>

namespace Moses2
{

template<class KeyClass, class ValueClass>
class Node
{
public:
  Node() {
  }
  Node(const ValueClass& value) :
    m_value(value) {
  }
  ~Node();
  void setKey(const KeyClass& key);
  void setValue(const ValueClass& value) {
    m_value = value;
  }
  Node* findSub(const KeyClass& key);
  const Node* findSub(const KeyClass& key) const;
  Node *addSubnode(const KeyClass& cKey) {
    Node *node = findSub(cKey);
    if (node) {
      return node;
    } else {
      node = new Node();
      subNodes[cKey] = node;
      return node;
    }
  }

  std::vector<Node*> getSubnodes();
  const ValueClass &getValue() const {
    return m_value;
  }

private:
  boost::unordered_map<KeyClass, Node*> subNodes;
  ValueClass m_value;

};

template<class KeyClass, class ValueClass>
Node<KeyClass, ValueClass>::~Node()
{
  typename boost::unordered_map<KeyClass, Node*>::iterator iter;
  for (iter = subNodes.begin(); iter != subNodes.end(); ++iter) {
    Node *node = iter->second;
    delete node;
  }
}

template<class KeyClass, class ValueClass>
const Node<KeyClass, ValueClass>* Node<KeyClass, ValueClass>::findSub(
  const KeyClass& cKey) const
{
  typename boost::unordered_map<KeyClass, Node*>::const_iterator iter;
  iter = subNodes.find(cKey);
  if (iter != subNodes.end()) {
    Node *node = iter->second;
    return node;
  }
  return NULL;
}

template<class KeyClass, class ValueClass>
Node<KeyClass, ValueClass>* Node<KeyClass, ValueClass>::findSub(
  const KeyClass& cKey)
{
  typename boost::unordered_map<KeyClass, Node*>::iterator iter;
  iter = subNodes.find(cKey);
  if (iter != subNodes.end()) {
    Node *node = iter->second;
    return node;
  }
  return NULL;
}

}

