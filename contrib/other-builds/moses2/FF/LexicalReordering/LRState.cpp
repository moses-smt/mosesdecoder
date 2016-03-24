/*
 * LRState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */
#include "LRState.h"

namespace Moses2 {

LRState::LRState(const LRModel &config,
		LRModel::Direction dir,
		size_t offset)
:m_configuration(config)
,m_direction(dir)
,m_offset(offset)
{
}

}

