/*
 * StaticData.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "Vocab.h"

class StaticData {
public:
	StaticData();
	virtual ~StaticData();

protected:
  Vocab m_vocab;
};

