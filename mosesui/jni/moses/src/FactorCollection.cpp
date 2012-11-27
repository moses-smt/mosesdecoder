// $Id: FactorCollection.cpp 2477 2009-08-07 16:47:54Z bhaddow $

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

namespace Moses
{
FactorCollection FactorCollection::s_instance;

void FactorCollection::LoadVocab(FactorDirection direction, FactorType factorType, const string &filePath)
{
	ifstream 	inFile(filePath.c_str());

	string line;
#ifdef WITH_THREADS   
	boost::upgrade_lock<boost::shared_mutex> lock(m_accessLock);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
#endif
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

bool FactorCollection::Exists(FactorDirection direction, FactorType factorType, const string &factorString)
{
#ifdef WITH_THREADS
	boost::shared_lock<boost::shared_mutex> lock(m_accessLock);
#endif   
	// find string id
	const string *ptrString=&(*m_factorStringCollection.insert(factorString).first);

	FactorSet::const_iterator iterFactor;
	Factor search(direction, factorType, ptrString); // id not used for searching

	iterFactor = m_collection.find(search);
	return iterFactor != m_collection.end();
}

const Factor *FactorCollection::AddFactor(FactorDirection direction
																				, FactorType 			factorType
																				, const string 		&factorString)
{
#ifdef WITH_THREADS
	boost::upgrade_lock<boost::shared_mutex> lock(m_accessLock);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
#endif
	// find string id
	const string *ptrString=&(*m_factorStringCollection.insert(factorString).first);
	pair<FactorSet::iterator, bool> ret = m_collection.insert( Factor(direction, factorType, ptrString, m_factorId) );
	if (ret.second)
		++m_factorId; // new factor, make sure next new factor has diffrernt id
		
	const Factor *factor = &(*ret.first);
	return factor;
}

FactorCollection::~FactorCollection()
{
	//FactorSet::iterator iter;
	//for (iter = m_collection.begin() ; iter != m_collection.end() ; iter++)
	//{
	//	delete (*iter);
	//}
}

TO_STRING_BODY(FactorCollection);

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

}


