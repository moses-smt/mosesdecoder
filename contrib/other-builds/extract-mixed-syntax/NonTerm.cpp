/*
 * NonTerm.cpp
 *
 *  Created on: 22 Feb 2014
 *      Author: hieu
 */

#include <sstream>
#include "NonTerm.h"
#include "Word.h"

using namespace std;

NonTerm::NonTerm(const ConsistentPhrase &consistentPhrase,
				const std::string &source,
				const std::string &target)
:m_consistentPhrase(consistentPhrase)
,m_source(source)
,m_target(target)
{
	// TODO Auto-generated constructor stub

}

NonTerm::~NonTerm() {
	// TODO Auto-generated destructor stub
}

std::string NonTerm::Debug() const
{
  stringstream out;
  out << m_source << m_target;
  return out.str();
}

void NonTerm::Output(std::ostream &out) const
{
  out << m_source << m_target;
}

