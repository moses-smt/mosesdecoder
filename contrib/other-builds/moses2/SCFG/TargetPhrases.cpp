/*
 * TargetPhrases.cpp
 *
 *  Created on: 15 Apr 2016
 *      Author: hieu
 */

#include <algorithm>
#include "TargetPhrases.h"
#include "../TargetPhrase.h"

namespace Moses2
{
namespace SCFG
{
TargetPhrases::TargetPhrases(MemPool &pool)
{
  m_coll = new (pool.Allocate<Coll>()) Coll(pool);
}

TargetPhrases::TargetPhrases(MemPool &pool, size_t size)
{
  m_coll = new (pool.Allocate<Coll>()) Coll(pool);
}

TargetPhrases::~TargetPhrases()
{
  // TODO Auto-generated destructor stub
}

void TargetPhrases::SortAndPrune(size_t tableLimit)
{
  iterator iterMiddle;
  iterMiddle =
      (tableLimit == 0 || m_coll->size() < tableLimit) ?
          m_coll->end() : m_coll->begin() + tableLimit;
/*
  std::partial_sort(m_coll.begin(), iterMiddle, m_coll.end(),
      CompareFutureScore<SCFG::Word>());
*/
  if (tableLimit && m_coll->size() > tableLimit) {
    m_coll->resize(tableLimit);
  }

  //cerr << "TargetPhrases=" << GetSize() << endl;
}

}
} /* namespace Moses2 */
