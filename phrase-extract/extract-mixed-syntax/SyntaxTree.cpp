#include <cassert>
#include <iostream>
#include "SyntaxTree.h"
#include "Parameter.h"

using namespace std;

void SyntaxTree::Add(int startPos, int endPos, const std::string &label, const Parameter &params)
{
  //cerr << "add " << label << " to " << "[" << startPos << "-" << endPos << "]" << endl;

  Range range(startPos, endPos);
  Labels &labels = m_coll[range];

  bool add = true;
  if (labels.size()) {
    if (params.multiLabel == 1) {
      // delete the label in collection and add new
      assert(labels.size() == 1);
      labels.clear();
    } else if (params.multiLabel == 2) {
      // ignore this label
      add = false;
    }
  }

  if (add) {
    labels.push_back(label);
  }
}

void SyntaxTree::AddToAll(const std::string &label)
{
  Coll::iterator iter;
  for (iter = m_coll.begin(); iter != m_coll.end(); ++iter) {
    Labels &labels = iter->second;
    labels.push_back(label);
  }
}

const SyntaxTree::Labels &SyntaxTree::Find(int startPos, int endPos) const
{
  Coll::const_iterator iter;
  iter = m_coll.find(Range(startPos, endPos));
  return (iter == m_coll.end()) ? m_defaultLabels : iter->second;
}
