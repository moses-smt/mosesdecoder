/*
 * Word.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#include <limits>
#include "Word.h"
#include "moses/Util.h"

using namespace std;

Word::Word(int pos, const std::string &str)
  :m_pos(pos)
  ,m_str(str)
{
  // TODO Auto-generated constructor stub

}

Word::~Word()
{
  // TODO Auto-generated destructor stub
}

void Word::AddAlignment(const Word *other)
{
  m_alignment.insert(other);
}

std::set<int> Word::GetAlignmentIndex() const
{
  std::set<int> ret;

  std::set<const Word *>::const_iterator iter;
  for (iter = m_alignment.begin(); iter != m_alignment.end(); ++iter) {
    const Word &otherWord = **iter;
    int otherPos = otherWord.GetPos();
    ret.insert(otherPos);
  }

  return ret;
}

void Word::Output(std::ostream &out) const
{
  out << m_str;
}

std::string Word::Debug() const
{
  return m_str;
}

int Word::CompareString(const Word &other) const
{
  return m_str.compare(other.m_str);
}

std::string Word::GetString(int factor) const
{
  vector<string> toks;
  Moses::Tokenize(toks, m_str, "|");

  assert(factor < toks.size());
  return toks[factor];
}


