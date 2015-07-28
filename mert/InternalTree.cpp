#include "InternalTree.h"

namespace MosesTuning
{

InternalTree::InternalTree(const std::string & line, const bool terminal):
  m_isTerminal(terminal)
{

  size_t found = line.find_first_of("[] ");

  if (found == line.npos) {
    m_value = line;
  }

  else {
    AddSubTree(line, 0);
  }
}

size_t InternalTree::AddSubTree(const std::string & line, size_t pos)
{

  std::string value;
  char token = 0;

  while (token != ']' && pos != std::string::npos) {
    size_t oldpos = pos;
    pos = line.find_first_of("[] ", pos);
    if (pos == std::string::npos) break;
    token = line[pos];
    value = line.substr(oldpos,pos-oldpos);

    if (token == '[') {
      if (m_value.size() > 0) {
        m_children.push_back(boost::make_shared<InternalTree>(value,false));
        pos = m_children.back()->AddSubTree(line, pos+1);
      } else {
        if (value.size() > 0) {
          m_value = value;
        }
        pos = AddSubTree(line, pos+1);
      }
    } else if (token == ' ' || token == ']') {
      if (value.size() > 0 && !(m_value.size() > 0)) {
        m_value = value;
      } else if (value.size() > 0) {
        m_isTerminal = false;
        m_children.push_back(boost::make_shared<InternalTree>(value,true));
      }
      if (token == ' ') {
        pos++;
      }
    }

    if (m_children.size() > 0) {
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


}
