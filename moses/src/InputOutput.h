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

/** Abstract class that represent a device which through the Moses library reads and writes data.
* The users of the library, eg. moses-cmd, should create a class which is inherited from this
* class
*/
class InputOutput
{
protected:
	long m_translationId;

	// constructor
	InputOutput();

	/** fill inputType (currently either a Sentence or ConfusionNet) by calling its Read() function.
	* Return the same inputType, or delete and return NULL if unsuccessful
	*/
	InputType* GetInput(InputType * inputType
										, std::istream &inputStream
										, const std::vector<FactorType> &factorOrder
										, FactorCollection &factorCollection);	

public:
	virtual ~InputOutput();

	/** return a sentence or confusion network with data read from file or stdin
	\param in empty InputType to be filled with data
	*/
	virtual InputType* GetInput(InputType *in) = 0;

	/** return the best translation in hypo, or NULL if no translation was possible
	\param hypo return arg of best translation found by decoder
	\param translationId id of the input
	\param reportSegmentation set to true if segmentation info required. Outputs to stdout
	\reportAllFactors output all factors, rather than just output factors. Not sure if needed now we know which output factors we want
	*/
	virtual void SetOutput(const Hypothesis *hypo, long translationId, bool reportSegmentation, bool reportAllFactors) = 0;

	/** return n-best list via the arg nBestList	*/
	virtual void SetNBest(const LatticePathList &nBestList, long translationId) = 0;

	//! delete InputType
	virtual void Release(InputType *inputType);

  void ResetTranslationId() { m_translationId = 0; }
};
