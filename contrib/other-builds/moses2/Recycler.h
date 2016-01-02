/*
 * Recycler.h
 *
 *  Created on: 2 Jan 2016
 *      Author: hieu
 */

#pragma once

#include <deque>

namespace Moses2 {

template<typename T>
class Recycler {
public:
	Recycler()
    {}
	virtual ~Recycler()
	{}

	bool empty() const
	{ return m_coll.empty(); }

	T &back()
	{ return m_coll.back(); }

	void pop_back()
	{ m_coll.pop_back(); }

	void clear()
	{ m_coll.clear(); }

	void push_back (const T& val)
	{ m_coll.push_back(val);	}

protected:
	std::deque<T> m_coll;
};

} /* namespace Moses2 */

