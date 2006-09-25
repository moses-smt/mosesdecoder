// $Id$
// vim:tabstop=2

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
#include "Sentence.h"
#include "PhraseDictionary.h"
#include "TranslationOptionCollectionText.h"
#include "StaticData.h"
#include "Util.h"

int Sentence::Read(std::istream& in,const std::vector<FactorType>& factorOrder,
									 FactorCollection &factorCollection) 
{
	const std::string& factorDelimiter = StaticData::Instance()->GetFactorDelimiter();
	std::string line;
	do 
		{
			if (getline(in, line, '\n').eof())	return 0;
			line = Trim(line);
		} while (line == "");
	
	Phrase::CreateFromString(factorOrder, line, factorCollection, factorDelimiter);
	return 1;
}

TargetPhraseCollection const* Sentence::
CreateTargetPhraseCollection(PhraseDictionaryBase const& d,
														 const WordsRange& r) const 
{
	Phrase src=GetSubString(r);
	return d.GetTargetPhraseCollection(src);
}

TranslationOptionCollection* 
Sentence::CreateTranslationOptionCollection() const 
{
	size_t maxNoTransOptPerCoverage = StaticData::Instance()->GetMaxNoTransOptPerCoverage();
	TranslationOptionCollection *rv= new TranslationOptionCollectionText(*this, maxNoTransOptPerCoverage);
	assert(rv);
	return rv;
}
void Sentence::Print(std::ostream& out) const
{
	out<<*static_cast<Phrase const*>(this)<<"\n";
}
