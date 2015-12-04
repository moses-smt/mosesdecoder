/*
 * Recycler.h
 *
 *  Created on: 5 Nov 2015
 *      Author: hieu
 */

#ifndef RECYCLER_H_
#define RECYCLER_H_

#include <vector>

template<typename T>
class Recycler {
	typedef std::vector<T> Coll;

public:
	  typedef typename Coll::iterator iterator;
	  typedef typename Coll::const_iterator const_iterator;
	  //! iterators
	  const_iterator begin() const {
		return m_coll.begin();
	  }
	  const_iterator end() const {
		return m_coll.end();
	  }

	  iterator begin() {
		return m_coll.begin();
	  }
	  iterator end() {
		return m_coll.end();
	  }

	Recycler()
	{
		m_coll.reserve(10000);
	}

	virtual ~Recycler()
	{}

	bool IsEmpty() const
	{ return m_coll.empty(); }

	T &Get()
	{ return m_coll.back(); }

	void Pop()
	{
		if (m_coll.size()) {
			m_coll.resize(m_coll.size() - 1);
		}
	}

	void Add(T &obj)
	{ m_coll.push_back(obj); }

	void Reset()
	{ m_coll.clear(); }
protected:
	Coll m_coll;
};

#endif /* RECYCLER_H_ */
