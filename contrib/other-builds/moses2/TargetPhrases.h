/*
 * TargetPhrases.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
#include "TargetPhrase.h"

class TargetPhrases {
	friend std::ostream& operator<<(std::ostream &, const TargetPhrases &);
	typedef std::vector<const TargetPhrase*> Coll;
public:
  typedef boost::shared_ptr<TargetPhrases> shared_ptr;
  typedef boost::shared_ptr<TargetPhrases const> shared_const_ptr;

  typedef Coll::iterator iterator;
  typedef Coll::const_iterator const_iterator;
  //! iterators
  const_iterator begin() const {
	return m_coll.begin();
  }
  const_iterator end() const {
	return m_coll.end();
  }

	TargetPhrases();
	virtual ~TargetPhrases();

	void AddTargetPhrase(const TargetPhrase &targetPhrase)
	{
		m_coll.push_back(&targetPhrase);
	}

	size_t GetSize() const
	{ return m_coll.size(); }

	const TargetPhrase& operator[](size_t ind) const
	{ return *m_coll[ind]; }

	void SortAndPrune(size_t tableLimit);

	const TargetPhrases *Clone() const;
protected:
	Coll m_coll;

};

