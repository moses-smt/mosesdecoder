#include "InternalTree.h"

namespace Moses
{

InternalTree::InternalTree(const std::string & line, size_t start, size_t len, const bool terminal):
  m_value_nt(0),
  m_isTerminal(terminal)
{

  if (len > 0) {
    m_value.assign(line, start, len);
  }
}

InternalTree::InternalTree(const std::string & line, const bool terminal):
  m_value_nt(0),
  m_isTerminal(terminal)
{

  size_t found = line.find_first_of("[] ");

  if (found == line.npos) {
    m_value = line;
  } else {
    AddSubTree(line, 0);
  }
}

size_t InternalTree::AddSubTree(const std::string & line, size_t pos)
{

  char token = 0;
  size_t len = 0;

  while (token != ']' && pos != std::string::npos) {
    size_t oldpos = pos;
    pos = line.find_first_of("[] ", pos);
    if (pos == std::string::npos) break;
    token = line[pos];
    len = pos-oldpos;

    if (token == '[') {
      if (!m_value.empty()) {
        m_children.push_back(boost::make_shared<InternalTree>(line, oldpos, len, false));
        pos = m_children.back()->AddSubTree(line, pos+1);
      } else {
        if (len > 0) {
          m_value.assign(line, oldpos, len);
        }
        pos = AddSubTree(line, pos+1);
      }
    } else if (token == ' ' || token == ']') {
      if (len > 0 && m_value.empty()) {
        m_value.assign(line, oldpos, len);
      } else if (len > 0) {
        m_isTerminal = false;
        m_children.push_back(boost::make_shared<InternalTree>(line, oldpos, len, true));
      }
      if (token == ' ') {
        pos++;
      }
    }

    if (!m_children.empty()) {
      m_isTerminal = false;
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

  if (!m_isTerminal) {
    ret += "[";
  }

  ret += m_value;
  for (std::vector<TreePointer>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
    ret += (*it)->GetString(false);
  }

  if (!m_isTerminal) {
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


bool InternalTree::FlatSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it) const
{
  for (it = m_children.begin(); it != m_children.end(); ++it) {
    if ((*it)->GetLabel() == label) {
      return true;
    }
  }
  return false;
}

bool InternalTree::RecursiveSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it) const
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

bool InternalTree::RecursiveSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it, InternalTree const* &parent) const
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


bool InternalTree::FlatSearch(const NTLabel & label, std::vector<TreePointer>::const_iterator & it) const
{
  for (it = m_children.begin(); it != m_children.end(); ++it) {
    if ((*it)->GetNTLabel() == label) {
      return true;
    }
  }
  return false;
}

bool InternalTree::RecursiveSearch(const NTLabel & label, std::vector<TreePointer>::const_iterator & it) const
{
  for (it = m_children.begin(); it != m_children.end(); ++it) {
    if ((*it)->GetNTLabel() == label) {
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

bool InternalTree::RecursiveSearch(const NTLabel & label, std::vector<TreePointer>::const_iterator & it, InternalTree const* &parent) const
{
  for (it = m_children.begin(); it != m_children.end(); ++it) {
    if ((*it)->GetNTLabel() == label) {
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


bool InternalTree::FlatSearch(const std::vector<NTLabel> & labels, std::vector<TreePointer>::const_iterator & it) const
{
  for (it = m_children.begin(); it != m_children.end(); ++it) {
    if (std::binary_search(labels.begin(), labels.end(), (*it)->GetNTLabel())) {
      return true;
    }
  }
  return false;
}

bool InternalTree::RecursiveSearch(const std::vector<NTLabel> & labels, std::vector<TreePointer>::const_iterator & it) const
{
  for (it = m_children.begin(); it != m_children.end(); ++it) {
    if (std::binary_search(labels.begin(), labels.end(), (*it)->GetNTLabel())) {
      return true;
    }
    std::vector<TreePointer>::const_iterator it2;
    if ((*it)->RecursiveSearch(labels, it2)) {
      it = it2;
      return true;
    }
  }
  return false;
}

bool InternalTree::RecursiveSearch(const std::vector<NTLabel> & labels, std::vector<TreePointer>::const_iterator & it, InternalTree const* &parent) const
{
  for (it = m_children.begin(); it != m_children.end(); ++it) {
    if (std::binary_search(labels.begin(), labels.end(), (*it)->GetNTLabel())) {
      parent = this;
      return true;
    }
    std::vector<TreePointer>::const_iterator it2;
    if ((*it)->RecursiveSearch(labels, it2, parent)) {
      it = it2;
      return true;
    }
  }
  return false;
}

}