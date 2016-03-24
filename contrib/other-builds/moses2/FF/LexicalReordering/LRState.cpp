/*
 * LRState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */
#include "LRState.h"

namespace Moses2 {

LRState::LRState(const LRModel &config,
		LRModel::Direction dir)
:m_configuration(config)
,m_direction(dir)
{
}

}

