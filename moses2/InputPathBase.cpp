/*
 * InputPath.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "InputPathBase.h"
#include "TranslationModel/PhraseTable.h"

namespace Moses2
{
InputPathBase::InputPathBase(MemPool &pool,
                             const Range &range, size_t numPt, const InputPathBase *prefixPath) :
  range(range), prefixPath(prefixPath)
{

}

}

