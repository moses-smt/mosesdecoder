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

	T &Get()
	{ return m_coll.back(); }

	void Pop()
	{ m_coll.pop_back(); }

	void clear()
	{ m_coll.clear(); }

	void Add (const T& val)
	{ m_coll.push_back(val);	}

protected:
	std::vector<T> m_all;
	std::deque<T> m_coll;
};

} /* namespace Moses2 */

