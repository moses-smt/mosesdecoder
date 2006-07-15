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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "FactorCollection.h"
#include "LanguageModel.h"
#include "Util.h"

using namespace std;

void FactorCollection::LoadVocab(FactorDirection direction, FactorType factorType, const string &fileName)
{
	ifstream 	inFile(fileName.c_str());

	string line;
	
	while( !getline(inFile, line, '\n').eof())
	{
		vector<string> token = Tokenize( line );
		if (token.size() < 2) 
		{
			continue;
		}		
		// looks like good line
		AddFactor(direction, factorType, token[1]);
	}
}

const Factor *FactorCollection::AddFactor(FactorDirection direction
																				, FactorType 			factorType
																				, const string 		&factorString
																				, LmId						lmId)
{
	// find string id
	const string *ptr=&(*m_factorStringCollection.insert(factorString).first);
//	Factor findFactor(direction, factorType, ptr, lmId);
	return &(*m_collection.insert(Factor(direction, factorType, ptr, lmId)).first);
}

const Factor *FactorCollection::AddFactor(FactorDirection direction
																				, FactorType 			factorType
																				, const string 		&factorString)
{
	return AddFactor(direction, factorType, factorString, LanguageModel::UNKNOWN_LM_ID);
}

void FactorCollection::SetFactorLmId(const Factor *factor, LmId lmId)
{ // only used by non-srilm code
	Factor *changeFactor = const_cast<Factor *>(factor);
	changeFactor->SetLmId(lmId);
}

FactorCollection::~FactorCollection()
{
	//FactorSet::iterator iter;
	//for (iter = m_collection.begin() ; iter != m_collection.end() ; iter++)
	//{
	//	delete (*iter);
	//}
}

// friend
ostream& operator<<(ostream& out, const FactorCollection& factorCollection)
{
	FactorSet::const_iterator iterFactor;

	for (iterFactor = factorCollection.m_collection.begin() ; iterFactor != factorCollection.m_collection.end() ; ++iterFactor)
	{
		const Factor &factor 	= *iterFactor;
		out << factor;
	}

	return out;
}

