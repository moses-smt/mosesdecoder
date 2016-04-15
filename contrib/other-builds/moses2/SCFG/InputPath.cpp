/*
 * InputPath.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "InputPath.h"
#include "../TranslationModel/PhraseTable.h"
#include "../MemPool.h"

namespace Moses2
{
namespace SCFG
{

InputPath::InputPath(MemPool &pool, const SubPhrase &subPhrase,
    const Range &range, size_t numPt, const InputPath *prefixPath) :
    InputPathBase(pool, subPhrase, range, numPt, prefixPath)
{
  m_activeChart = pool.Allocate<ActiveChart>(numPt);
  for (size_t i = 0; i < numPt; ++i) {
    ActiveChart &memAddr = m_activeChart[i];
    ActiveChart *obj = new (&memAddr) ActiveChart();
  }

  targetPhrases = pool.Allocate<const TargetPhrases*>(numPt);
  Init<const TargetPhrases*>(targetPhrases, numPt, NULL);
}

InputPath::~InputPath()
{
  // TODO Auto-generated destructor stub
}

std::ostream& operator<<(std::ostream &out, const InputPath &obj)
{
  out << obj.range << " " << obj.subPhrase;
  return out;
}

}
}

