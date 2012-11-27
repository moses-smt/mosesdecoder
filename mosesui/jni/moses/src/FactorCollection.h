// $Id: FactorCollection.h 2939 2010-02-24 11:15:44Z jfouet $

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

#ifndef moses_FactorCollection_h
#define moses_FactorCollection_h

#include <set>
#include <string>

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#endif

#include "Factor.h"

namespace Moses
{

class LanguageModel;

typedef std::set<Factor> FactorSet;
typedef std::set<std::string> StringSet;

/** collection of factors
 *
 * All Factors in moses are accessed and created by a FactorCollection.
 * By enforcing this strict creation processes (ie, forbidding factors
 * from being created on the stack, etc), their memory addresses can
 * be used as keys to uniquely identify them.
 * Only 1 FactorCollection object should be created.
 */
class FactorCollection
{
	friend std::ostream& operator<<(std::ostream&, const FactorCollection&);

protected:
	static FactorCollection s_instance;
#ifdef WITH_THREADS   
    //reader-writer lock
    boost::shared_mutex m_accessLock;
#endif

	size_t		m_factorId; /**< unique, contiguous ids, starting from 0, for each factor */	
	FactorSet m_collection; /**< collection of all factors */
	StringSet m_factorStringCollection; /**< collection of unique string used by factors */

	//! constructor. only the 1 static variable can be created
	FactorCollection()
	:m_factorId(0)
	{}

public:		
	static FactorCollection& Instance() { return s_instance; }

	//! Destructor
	~FactorCollection();

	//! Test to see whether a factor exists
	bool Exists(FactorDirection direction, FactorType factorType, const std::string &factorString);	
	/** returns a factor with the same direction, factorType and factorString. 
	*	If a factor already exist in the collection, return the existing factor, if not create a new 1
	*/
	const Factor *AddFactor(FactorDirection direction, FactorType factorType, const std::string &factorString);	
	//! Load list of factors. Deprecated
	void LoadVocab(FactorDirection direction, FactorType factorType, const std::string &filePath);
	
	TO_STRING();
	
};


}
#endif
