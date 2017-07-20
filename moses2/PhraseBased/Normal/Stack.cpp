/*
 * Stack.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "Stack.h"
#include "../Hypothesis.h"
#include "../Manager.h"
#include "../../Scores.h"
#include "../../HypothesisColl.h"

using namespace std;

namespace Moses2
{

namespace NSNormal
{

Stack::Stack(const Manager &mgr) :
  HypothesisColl(mgr)
{
  // TODO Auto-generated constructor stub

}

Stack::~Stack()
{
  // TODO Auto-generated destructor stub
}

}
}
