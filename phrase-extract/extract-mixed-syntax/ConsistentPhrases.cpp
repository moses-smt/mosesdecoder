/*
 * ConsistentPhrases.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */
#include <sstream>
#include <cassert>
#include "ConsistentPhrases.h"
#include "NonTerm.h"
#include "Parameter.h"
#include "moses/Util.h"

using namespace std;

ConsistentPhrases::ConsistentPhrases()
{
}

ConsistentPhrases::~ConsistentPhrases()
{
  for (int start = 0; start < m_coll.size(); ++start) {
    std::vector<Coll> &allSourceStart = m_coll[start];

    for (int size = 0; size < allSourceStart.size(); ++size) {
      Coll &coll = allSourceStart[size];
      Moses::RemoveAllInColl(coll);
    }
  }
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
                            int targetStart, int targetEnd,
                            const Parameter &params)
{
  Coll &coll = m_coll[sourceStart][sourceEnd - sourceStart];
  ConsistentPhrase *cp = new ConsistentPhrase(sourceStart, sourceEnd,
      targetStart, targetEnd,
      params);

  pair<Coll::iterator, bool> inserted = coll.insert(cp);
  assert(inserted.second);
}

const ConsistentPhrases::Coll &ConsistentPhrases::GetColl(int sourceStart, int sourceEnd) const
{
  const std::vector<Coll> &allSourceStart = m_coll[sourceStart];
  const Coll &ret = allSourceStart[sourceEnd - sourceStart];
  return ret;
}

ConsistentPhrases::Coll &ConsistentPhrases::GetColl(int sourceStart, int sourceEnd)
{
  std::vector<Coll> &allSourceStart = m_coll[sourceStart];
  Coll &ret = allSourceStart[sourceEnd - sourceStart];
  return ret;
}

std::string ConsistentPhrases::Debug() const
{
  std::stringstream out;
  for (int start = 0; start < m_coll.size(); ++start) {
    const std::vector<Coll> &allSourceStart = m_coll[start];

    for (int size = 0; size < allSourceStart.size(); ++size) {
      const Coll &coll = allSourceStart[size];

      Coll::const_iterator iter;
      for (iter = coll.begin(); iter != coll.end(); ++iter) {
        const ConsistentPhrase &consistentPhrase = **iter;
        out << consistentPhrase.Debug() << endl;
      }
    }
  }

  return out.str();
}

void ConsistentPhrases::AddHieroNonTerms(const Parameter &params)
{
  // add [X] labels everywhere
  for (int i = 0; i < m_coll.size(); ++i) {
    vector<Coll> &inner = m_coll[i];
    for (int j = 0; j < inner.size(); ++j) {
      ConsistentPhrases::Coll &coll = inner[j];
      ConsistentPhrases::Coll::iterator iter;
      for (iter = coll.begin(); iter != coll.end(); ++iter) {
        ConsistentPhrase &cp = **iter;
        cp.AddNonTerms(params.hieroNonTerm, params.hieroNonTerm);
      }
    }
  }
}

