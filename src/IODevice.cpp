// $Id: IODevice.cpp 905 2006-10-21 16:21:21Z hieuhoang1972 $

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

#include "IODevice.h"
#include "InputType.h"

IODevice::IODevice() : m_translationId(0) {}

IODevice::~IODevice() {}

void IODevice::Release(InputType *s) {delete s;}


InputType* IODevice::GetInput(InputType *inputType
																 , std::istream &inputStream
																 , const std::vector<FactorType> &factorOrder
																 , FactorCollection &factorCollection) 
{
	if(inputType->Read(inputStream,factorOrder,factorCollection)) 
		{
			inputType->SetTranslationId(m_translationId++);
			return inputType;
		}
	else 
		{
			delete inputType;
			return NULL;
		}
}

