/*
 * InputPaths.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "../InputPathsBase.h"

namespace Moses2
{

class Sentence;
class System;

namespace SCFG
{

class InputPaths : public InputPathsBase
{
public:
	  void Init(const Sentence &input, const ManagerBase &mgr);


protected:

};

}
}

