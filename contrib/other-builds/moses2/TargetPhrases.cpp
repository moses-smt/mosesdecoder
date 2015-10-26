/*
 * TargetPhrases.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "TargetPhrases.h"

using namespace std;

TargetPhrases::TargetPhrases() {
	// TODO Auto-generated constructor stub

}

TargetPhrases::~TargetPhrases() {
	// TODO Auto-generated destructor stub
}

std::ostream& operator<<(std::ostream &out, const TargetPhrases &obj)
{
	BOOST_FOREACH(const TargetPhrase *tp, obj) {
		out << *tp << endl;
	}

	return out;
}
