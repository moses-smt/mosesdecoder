/*
 * Recycler.h
 *
 *  Created on: 5 Nov 2015
 *      Author: hieu
 */

#ifndef RECYCLER_H_
#define RECYCLER_H_

#include <deque>

namespace Moses2
{

template<typename T>
class Recycler {
	typedef std::deque<T> Coll;

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
	}

	virtual ~Recycler()
	{}

	bool empty() const
	{ return m_coll.empty(); }

	T &back()
	{ return m_coll.back(); }

	void pop_back()
	{
		m_coll.pop_back();
	}

	void push_back(T &obj)
	{ m_coll.push_back(obj); }

	void clear()
	{ m_coll.clear(); }
protected:
	Coll m_coll;
};

}


#endif /* RECYCLER_H_ */
