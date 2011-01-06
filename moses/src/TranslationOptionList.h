#ifndef moses_TranslationOptionList_h
#define moses_TranslationOptionList_h

#include <vector>
#include <cassert>
#include <iostream>
#include "Util.h"

namespace Moses
{

class TranslationOption;

class TranslationOptionList
{
	friend std::ostream& operator<<(std::ostream& out, const TranslationOptionList& coll);

protected:
	typedef std::vector<TranslationOption*> CollType;
	CollType m_coll;

	public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }

	TranslationOptionList()
	{
	}
	TranslationOptionList(const TranslationOptionList &copy);
	~TranslationOptionList();

	void resize(size_t newSize)
	{ m_coll.resize(newSize); }
	size_t size() const
	{ return m_coll.size(); }

	const TranslationOption *Get(size_t ind) const
	{
		assert(ind < m_coll.size());
		return m_coll[ind];
	}
	void Remove( size_t ind )
	{
		assert(ind < m_coll.size());
		m_coll.erase( m_coll.begin()+ind );
	}
	void Add(TranslationOption *transOpt)
	{
		m_coll.push_back(transOpt);
	}

	TO_STRING();		

};

}

#endif
