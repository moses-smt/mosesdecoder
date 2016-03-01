/*
 * InputPath.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <iostream>
#include <vector>
#include "../InputPathBase.h"

namespace Moses2
{
namespace SCFG
{
class SCFGPath
{
public:
	//Phrase ruleSource;
};

class SCFGPaths
{
public:
	//Phrase ruleSource;
};

class InputPath : public InputPathBase
{
	  friend std::ostream& operator<<(std::ostream &, const InputPath &);
public:
	InputPath(MemPool &pool, const SubPhrase &subPhrase, const Range &range, size_t numPt, const InputPath *prefixPath);
	virtual ~InputPath();


protected:
	SCFGPaths *m_scfgPaths;
};

}
}

