// $Id: LanguageModelImplementation.cpp 3719 2010-11-17 14:06:21Z chardmeier $

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

#include <cassert>
#include <limits>
#include <iostream>
#include <memory>
#include <sstream>

#include "FFState.h"
#include "LanguageModelImplementation.h"
#include "TypeDef.h"
#include "Util.h"
#include "Manager.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{
float LanguageModelImplementation::GetValueGivenState(
	const std::vector<const Word*> &contextFactor,
	FFState &state,
	unsigned int* len) const
{
	return GetValueForgotState(contextFactor, state, len);
}

void LanguageModelImplementation::GetState(
	const std::vector<const Word*> &contextFactor,
	FFState &state) const
{
	GetValueForgotState(contextFactor, state, NULL);
}
	
}
