// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

/*
* The DistortionOrientation class contains a static method which returns 
* what DistortionOrientation type (one of {MONO, NON_MONO, SWAP, DISC }
* as enumerated under ORIENTATIONS in TypeDef.h) the current phrase
* is with respect to the previous.
*/

#pragma once

#include <string>
#include <vector>
#include <map>
#include "TypeDef.h"
#include "WordsRange.h"
#include "Hypothesis.h"


class DistortionOrientation
{
    public:
		static int GetOrientation(const Hypothesis *curr_hypothesis, int direction, int type=DistortionOrientationType::Msd);
};

