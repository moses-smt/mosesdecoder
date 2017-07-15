/*
 * InputPath.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <iostream>
#include <vector>
#include "SubPhrase.h"
#include "legacy/Range.h"

namespace Moses2
{

class PhraseTable;

class InputPathBase
{
public:
  const InputPathBase *prefixPath;
  Range range;

  InputPathBase(MemPool &pool, const Range &range,
                size_t numPt, const InputPathBase *prefixPath);

};

}

