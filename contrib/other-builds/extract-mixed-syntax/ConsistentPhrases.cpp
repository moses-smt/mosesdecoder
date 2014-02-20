/*
 * ConsistentPhrases.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */
#include <ConsistentPhrases.h>

using namespace std;

ConsistentPhrases::ConsistentPhrases() {
	// TODO Auto-generated constructor stub

}

ConsistentPhrases::~ConsistentPhrases() {
	// TODO Auto-generated destructor stub
}

void ConsistentPhrases::Add(int sourceStart, int sourceEnd,
		int targetStart, int targetEnd)
{
	m_coll.insert(ConsistentPhrase(sourceStart,
					sourceEnd,
					targetStart,
					targetEnd));
}

void ConsistentPhrases::Debug(std::ostream &out) const
{
	Coll::const_iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter) {
		const ConsistentPhrase &consistentPhrase = *iter;
		consistentPhrase.Debug(out);
		out << endl;
	}
}
