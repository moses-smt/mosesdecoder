
#include "Node.h"
#include "Phrase.h"

Node::Node()
{
  // TODO Auto-generated constructor stub

}

Node::~Node()
{
  // TODO Auto-generated destructor stub
}

Node &Node::GetOrCreate(const Phrase &source, size_t pos)
{
  if (pos == source.GetSize()) {
    return *this;
  }

  const Word &word = source.GetWord(pos);
  Node &child = m_children[word];
  return child.GetOrCreate(source, pos + 1);
}

const Node *Node::Get(const Word &word) const
{
  Children::const_iterator iter;
  iter = m_children.find(word);
  if (iter == m_children.end()) {
    return NULL;
  }

  // found child node
  const Node &child = iter->second;
  return &child;
}

void Node::AddTarget(TargetPhrase *target)
{
  m_tpColl.Add(target);
}
