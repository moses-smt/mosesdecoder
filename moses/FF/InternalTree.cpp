#include "InternalTree.h"
#include "moses/StaticData.h"

namespace Moses
{

InternalTree::InternalTree(const std::string & line, size_t start, size_t len, const bool nonterminal)
{

  std::vector<FactorType> const& oFactors
  = StaticData::Instance().options()->output.factor_order;
  if (len > 0) {
    m_value.CreateFromString(Output, oFactors, StringPiece(line).substr(start, len),
                             nonterminal);
  }
}

InternalTree::InternalTree(const std::string & line, const bool nonterminal)
{

  size_t found = line.find_first_of("[] ");

  if (found == line.npos) {
    m_value.CreateFromString(Output,
                             StaticData::Instance().options()->output.factor_order,
                             line, nonterminal);
  } else {
    AddSubTree(line, 0);
  }
}

size_t InternalTree::AddSubTree(const std::string & line, size_t pos)
{

  char token = 0;
  size_t len = 0;
  bool has_value = false;

  while (token != ']' && pos != std::string::npos) {
    size_t oldpos = pos;
    pos = line.find_first_of("[] ", pos);
    if (pos == std::string::npos) break;
    token = line[pos];
    len = pos-oldpos;

    if (token == '[') {
      if (has_value) {
        m_children.push_back(boost::make_shared<InternalTree>(line, oldpos, len, true));
        pos = m_children.back()->AddSubTree(line, pos+1);
      } else {
        if (len > 0) {
          m_value.CreateFromString(Output,
                                   StaticData::Instance().options()->output.factor_order,
                                   StringPiece(line).substr(oldpos, len), false);
          has_value = true;
        }
        pos = AddSubTree(line, pos+1);
      }
    } else if (token == ' ' || token == ']') {
      if (len > 0 && !has_value) {
        m_value.CreateFromString(Output,
                                 StaticData::Instance().options()->output.factor_order,
                                 StringPiece(line).substr(oldpos, len), true);
        has_value = true;
      } else if (len > 0) {
        m_children.push_back(boost::make_shared<InternalTree>(line, oldpos, len, false));
      }
      if (token == ' ') {
        pos++;
      }
    }
  }

  if (pos == std::string::npos) {
    return line.size();
  }
  return std::min(line.size(),pos+1);

}

std::string InternalTree::GetString(bool start) const
{

  std::string ret = "";
  if (!start) {
    ret += " ";
  }

  if (!IsTerminal()) {
    ret += "[";
  }

  ret += m_value.GetString(StaticData::Instance().options()->output.factor_order, false);
  for (std::vector<TreePointer>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
    ret += (*it)->GetString(false);
  }

  if (!IsTerminal()) {
    ret += "]";
  }
  return ret;

}


void InternalTree::Combine(const std::vector<TreePointer> &previous)
{

  std::vector<TreePointer>::iterator it;
  bool found = false;
  leafNT next_leafNT(this);
  for (std::vector<TreePointer>::const_iterator it_prev = previous.begin(); it_prev != previous.end(); ++it_prev) {
    found = next_leafNT(it);
    if (found) {
      *it = *it_prev;
    } else {
      std::cerr << "Warning: leaf nonterminal not found in rule; why did this happen?\n";
    }
  }
}

//take tree with virtual nodes (created with relax-parse --RightBinarize or --LeftBinarize) and reconstruct original tree.
void InternalTree::Unbinarize()
{

  // nodes with virtual label cannot be unbinarized
  if (m_value.GetString(0).empty() || m_value.GetString(0).as_string()[0] == '^') {
    return;
  }

  //if node has child that is virtual node, get unbinarized list of children
  for (std::vector<TreePointer>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
    if (!(*it)->IsTerminal() && (*it)->GetLabel().GetString(0).as_string()[0] == '^') {
      std::vector<TreePointer> new_children;
      GetUnbinarizedChildren(new_children);
      m_children = new_children;
      break;
    }
  }

  //recursion
  for (std::vector<TreePointer>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
    (*it)->Unbinarize();
  }
}

//get the children of a node in a binarized tree; if a child is virtual, (transitively) replace it with its children
void InternalTree::GetUnbinarizedChildren(std::vector<TreePointer> &ret) const
{
  for (std::vector<TreePointer>::const_iterator itx = m_children.begin(); itx != m_children.end(); ++itx) {
    const StringPiece label = (*itx)->GetLabel().GetString(0);
    if (!label.empty() && label.as_string()[0] == '^') {
      (*itx)->GetUnbinarizedChildren(ret);
    } else {
      ret.push_back(*itx);
    }
  }
}

bool InternalTree::FlatSearch(const Word & label, std::vector<TreePointer>::const_iterator & it) const
{
  for (it = m_children.begin(); it != m_children.end(); ++it) {
    if ((*it)->GetLabel() == label) {
      return true;
    }
  }
  return false;
}

bool InternalTree::RecursiveSearch(const Word & label, std::vector<TreePointer>::const_iterator & it) const
{
  for (it = m_children.begin(); it != m_children.end(); ++it) {
    if ((*it)->GetLabel() == label) {
      return true;
    }
    std::vector<TreePointer>::const_iterator it2;
    if ((*it)->RecursiveSearch(label, it2)) {
      it = it2;
      return true;
    }
  }
  return false;
}

bool InternalTree::RecursiveSearch(const Word & label, std::vector<TreePointer>::const_iterator & it, InternalTree const* &parent) const
{
  for (it = m_children.begin(); it != m_children.end(); ++it) {
    if ((*it)->GetLabel() == label) {
      parent = this;
      return true;
    }
    std::vector<TreePointer>::const_iterator it2;
    if ((*it)->RecursiveSearch(label, it2, parent)) {
      it = it2;
      return true;
    }
  }
  return false;
}

}
