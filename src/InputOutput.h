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

#pragma once

#include <string>
#include <iostream>
#include <vector>
#include "TypeDef.h"

class Hypothesis;
class LatticePathList;
class FactorCollection;
class InputType;

class InputOutput
{
 protected:
	long sentenceId;
 public:
	InputOutput();
	virtual InputType* GetInput(InputType*) = 0;
	virtual void SetOutput(const Hypothesis *hypo, long translationId, bool reportSourceSpan) = 0;
	virtual void SetNBest(const LatticePathList &nBestList, long translationId) = 0;
	virtual ~InputOutput();
	virtual void Release(InputType*);

 protected:
	InputType* GetInput(InputType * inputType
											, std::istream &inputStream
											, const std::vector<FactorType> &factorOrder
											, FactorCollection &factorCollection);	
};
