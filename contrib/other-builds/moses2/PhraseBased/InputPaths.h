/*
 * InputPaths.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "InputPath.h"
#include "../MemPool.h"
#include "../InputPathsBase.h"
#include "../legacy/Matrix.h"

namespace Moses2
{

class Sentence;
class System;
class Manager;

class InputPaths : public InputPathsBase
{
public:
	  void Init(const Sentence &input, const Manager &mgr);

	  const InputPath &GetBlank() const
	  { return *m_blank; }

protected:
		InputPath *m_blank;

};

}


