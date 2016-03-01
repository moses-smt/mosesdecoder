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

InputPath::InputPath(MemPool &pool,
		const SubPhrase &subPhrase,
		const Range &range,
		size_t numPt,
		const InputPath *prefixPath)
:InputPathBase(pool, subPhrase, range, numPt, prefixPath)
{
  m_scfgPaths = pool.Allocate<SCFGPaths>(numPt);

}

InputPath::~InputPath() {
	// TODO Auto-generated destructor stub
}

std::ostream& operator<<(std::ostream &out, const InputPath &obj)
{
	out << obj.range << " " << obj.subPhrase;
	return out;
}

}
}


