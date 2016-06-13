/*
 * TargetPhrases.cpp
 *
 *  Created on: 15 Apr 2016
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include <algorithm>
#include "TargetPhrases.h"
#include "TargetPhraseImpl.h"
#include "../TargetPhrase.h"
#include "../TranslationModel/PhraseTable.h"

namespace Moses2
{
namespace SCFG
{
TargetPhrases::TargetPhrases(MemPool &pool)
:m_coll(pool)
{
}

TargetPhrases::TargetPhrases(MemPool &pool, size_t size)
:m_coll(pool)
{
  m_coll.reserve(size);
}

TargetPhrases::~TargetPhrases()
{
  // TODO Auto-generated destructor stub
}

void TargetPhrases::SortAndPrune(size_t tableLimit)
{
  iterator iterMiddle;
  iterMiddle =
      (tableLimit == 0 || m_coll.size() < tableLimit) ?
          m_coll.end() : m_coll.begin() + tableLimit;

  std::partial_sort(m_coll.begin(), iterMiddle, m_coll.end(),
      Compare1Score<SCFG::TargetPhraseImpl>());

  if (tableLimit && m_coll.size() > tableLimit) {
    m_coll.resize(tableLimit);
  }

  //cerr << "TargetPhrases=" << GetSize() << endl;
}

void TargetPhrases::Debug(std::ostream &out, const System &system) const
{
  BOOST_FOREACH(const SCFG::TargetPhraseImpl *tp, m_coll) {
    out << std::endl;
    tp->Debug(out, system);
  }
}

}
} /* namespace Moses2 */
