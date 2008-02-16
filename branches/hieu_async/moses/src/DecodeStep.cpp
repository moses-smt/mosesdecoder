// $Id: DecodeStep.cpp 139 2007-10-11 21:25:17Z hieu $

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

#include "DecodeStep.h"
#include "PhraseDictionaryMemory.h"
#include "GenerationDictionary.h"
#include "StaticData.h"

DecodeStep::DecodeStep()
{
}

void DecodeStep::SetDictionary(Dictionary *ptr)
{
	m_ptr = ptr;
}

DecodeStep::~DecodeStep() 
{
	delete m_ptr;
}

void DecodeStep::InitializeForInput(InputType const &source) const
{
	m_ptr->InitializeForInput(source);
}

void DecodeStep::CleanUp() const
{
	m_ptr->CleanUp();
}
