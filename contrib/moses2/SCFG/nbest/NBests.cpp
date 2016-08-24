/*
 * NBests.cpp
 *
 *  Created on: 24 Aug 2016
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "NBests.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

/////////////////////////////////////////////////////////////
NBests::~NBests()
{
	BOOST_FOREACH(const NBest *nbest, m_coll) {
		delete nbest;
	}

	// delete bad contenders left in queue
	while (!contenders.empty()) {
		NBest *contender = contenders.top();
		contenders.pop();
		delete contender;
	}
}


}
}

