/*
 * Word.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#include <limits>
#include "Word.h"

using namespace std;

Word::Word(int pos, const std::string &str)
:m_pos(pos)
,m_str(str)
,m_lowestAlignment(numeric_limits<int>::max())
,m_highestAlignment(-1)
{
	// TODO Auto-generated constructor stub

}

Word::~Word() {
	// TODO Auto-generated destructor stub
}

void Word::AddAlignment(int align)
{
	m_alignment.insert(align);
	if (align > m_highestAlignment) {
		m_highestAlignment = align;
	}
	if (align < m_lowestAlignment) {
		m_lowestAlignment = align;
	}
}

void Word::Output(std::ostream &out) const
{
	out << m_str;
}
