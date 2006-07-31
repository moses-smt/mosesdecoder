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

#ifdef WIN32


#include <sstream>
#include "ConfusionNet.h"
#include "FactorCollection.h"
#include "Util.h"

// for Windows. not implemented
#pragma warning(disable:4716)

ConfusionNet::ConfusionNet(FactorCollection* p) : InputType(),m_factorCollection(p) 
{
	assert(false);
}

ConfusionNet::~ConfusionNet()
{
	assert(false);
}

void ConfusionNet::SetFactorCollection(FactorCollection *p) 
{
	assert(false);
}
bool ConfusionNet::ReadF(std::istream& in,const std::vector<FactorType>& factorOrder,int format) {
	assert(false);
}

int ConfusionNet::Read(std::istream& in,const std::vector<FactorType>& factorOrder, FactorCollection &factorCollection) 
{
	assert(false);
}


void ConfusionNet::String2Word(const std::string& s,Word& w,const std::vector<FactorType>& factorOrder) {
	assert(false);
}

bool ConfusionNet::ReadFormat0(std::istream& in,const std::vector<FactorType>& factorOrder) {
	assert(false);
}
bool ConfusionNet::ReadFormat1(std::istream& in,const std::vector<FactorType>& factorOrder) {
	assert(false);
}

void ConfusionNet::Print(std::ostream& out) const {
	assert(false);
}

Phrase ConfusionNet::GetSubString(const WordsRange&) const {
	assert(false);
}
const FactorArray& ConfusionNet::GetFactorArray(size_t) const {
	assert(false);
}

std::ostream& operator<<(std::ostream& out,const ConfusionNet& cn) 
{
	assert(false);
}

TargetPhraseCollection const* ConfusionNet::CreateTargetPhraseCollection(PhraseDictionaryBase const& d,const WordsRange& r) const 
{
	assert(false);
}
TranslationOptionCollection* ConfusionNet::CreateTranslationOptionCollection() const 
{
	assert(false);
}

#pragma warning(default:4716)

#endif