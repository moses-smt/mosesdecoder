/*
 * NonTerm.cpp
 *
 *  Created on: 22 Feb 2014
 *      Author: hieu
 */

#include <sstream>
#include "NonTerm.h"
#include "Word.h"
#include "ConsistentPhrase.h"
#include "Parameter.h"

using namespace std;

NonTerm::NonTerm(const ConsistentPhrase &consistentPhrase,
                 const std::string &source,
                 const std::string &target)
  :m_consistentPhrase(&consistentPhrase)
  ,m_source(source)
  ,m_target(target)
{
  // TODO Auto-generated constructor stub

}

NonTerm::~NonTerm()
{
  // TODO Auto-generated destructor stub
}

std::string NonTerm::Debug() const
{
  stringstream out;
  out << m_source << m_target;
  out << m_consistentPhrase->Debug();
  return out.str();
}

void NonTerm::Output(std::ostream &out) const
{
  out << m_source << m_target;
}

void NonTerm::Output(std::ostream &out, Moses::FactorDirection direction) const
{
  out << GetLabel(direction);
}

const std::string &NonTerm::GetLabel(Moses::FactorDirection direction) const
{
  return (direction == Moses::Input) ? m_source : m_target;
}

bool NonTerm::IsHiero(Moses::FactorDirection direction, const Parameter &params) const
{
  const std::string &label = NonTerm::GetLabel(direction);
  return label == params.hieroNonTerm;
}

bool NonTerm::IsHiero(const Parameter &params) const
{
  return IsHiero(Moses::Input, params) && IsHiero(Moses::Output, params);
}

int NonTerm::GetWidth(Moses::FactorDirection direction) const
{
  return GetConsistentPhrase().GetWidth(direction);
}
