/*
 * ConsistentPhrases.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */
#include <ConsistentPhrases.h>
#include <cassert>

using namespace std;

ConsistentPhrases::ConsistentPhrases()
{
}

ConsistentPhrases::~ConsistentPhrases() {
	// TODO Auto-generated destructor stub
}

void ConsistentPhrases::Initialize(size_t size)
{
	m_coll.resize(size);

	for (size_t sourceStart = 0; sourceStart < size; ++sourceStart) {
		std::vector<Coll> &allSourceStart = m_coll[sourceStart];
		allSourceStart.resize(size - sourceStart);
	}
}

void ConsistentPhrases::Add(int sourceStart, int sourceEnd,
		int targetStart, int targetEnd)
{
  Coll &coll = m_coll[sourceStart][sourceEnd - sourceEnd];
  pair<Coll::iterator, bool> inserted = coll.insert(ConsistentPhrase(sourceStart,
					sourceEnd,
					targetStart,
					targetEnd));
  assert(inserted.second);

  //const ConsistentPhrase &cp = *inserted;

  //m_coll[sourceStart][sourceEnd - sourceStart] = &cp;
}

const ConsistentPhrases::Coll &ConsistentPhrases::GetColl(int sourceStart, int sourceEnd) const
{
	const std::vector<Coll> &allSourceStart = m_coll[sourceStart];
	const Coll &ret = allSourceStart[sourceEnd - sourceStart];
	return ret;
}

void ConsistentPhrases::Debug(std::ostream &out) const
{
	for (int start = 0; start < m_coll.size(); ++start) {
		const std::vector<Coll> &allSourceStart = m_coll[start];

		for (int size = 0; size < allSourceStart.size(); ++size) {
			const Coll &coll = allSourceStart[size];

			Coll::const_iterator iter;
			for (iter = coll.begin(); iter != coll.end(); ++iter) {
				const ConsistentPhrase &consistentPhrase = *iter;
				consistentPhrase.Debug(out);
				out << endl;
			}

		}
	}
}
