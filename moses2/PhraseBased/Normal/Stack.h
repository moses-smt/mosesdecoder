/*
 * Stack.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <boost/unordered_set.hpp>
#include <deque>
#include "../Hypothesis.h"
#include "../../TypeDef.h"
#include "../../HypothesisColl.h"
#include "../../legacy/Util2.h"

namespace Moses2
{

namespace NSNormal
{
class Stack: public HypothesisColl
{
public:
  Stack(const Manager &mgr);
  virtual ~Stack();

protected:

};

}
}
