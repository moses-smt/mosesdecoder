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

#ifdef USE_BOOST
#include <boost/unordered_set.hpp>
#include <boost/functional/hash.hpp>
#endif

#include <set>
#include <string>
#include "Factor.h"

namespace Moses
{
#ifdef USE_BOOST
class FactorHasher
{
public:
	size_t operator()(const Factor &factor) const
	{
		const std::string &str = factor.GetString();
		size_t hash = boost::hash_value(str);
		return hash;
	}
};
typedef boost::unordered_set<Factor, FactorHasher> FactorSet;

#else

typedef std::set<Factor> FactorSet;

#endif


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
	friend class LanguageModelSRI;
	friend class LanguageModelIRST;
	friend class LanguageModelJoint;
	friend class LanguageModelInternal;
	friend class LanguageModelSkip;
	friend class Sentence;
	friend class TranslationOptionCollection;
	friend class Word;
	friend class ChartInput;
	friend class PhraseDictionaryOnDisk;
	friend class PhraseDictionarySourceLabel;
	friend class PhraseDictionaryNewFormat;
	
protected:
	static FactorCollection s_instance;

	size_t		m_factorId; /**< unique, contiguous ids, starting from 0, for each factor */
	FactorSet m_collection; /**< collection of all factors */

	//! constructor. only the 1 static variable can be created
	FactorCollection()
	:m_factorId(0)
	{}
	static FactorCollection& Instance() { return s_instance; }

public:

	//! Destructor
	~FactorCollection();

	//! Test to see whether a factor exists
	bool Exists(FactorDirection direction, FactorType factorType, const std::string &factorString);
	/** returns a factor with the same direction, factorType and factorString.
	*	If a factor already exist in the collection, return the existing factor, if not create a new 1
	*/
	const Factor *AddFactor(FactorDirection direction, FactorType factorType, const std::string &factorString);
	const Factor *AddFactor(FactorDirection direction, FactorType factorType, const std::string &factorString, bool isNonTerminal);
	//! Load list of factors. Deprecated
	void LoadVocab(FactorDirection direction, FactorType factorType, const std::string &filePath);

	TO_STRING();

};


}
