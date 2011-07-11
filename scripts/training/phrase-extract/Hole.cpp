/*
 *  Hole.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "Hole.h"

bool Hole::IsSyntax() const
{
	bool ret = m_label[0] != "X" || m_label[1] != "X";
	return ret;
}


