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
public:
	Recycler()
	{
		m_coll.reserve(10000);
	}

	virtual ~Recycler()
	{}

	bool empty() const
	{ return m_coll.empty(); }

	T &front()
	{ return m_coll.back(); }

	void pop()
	{
		if (m_coll.size()) {
			m_coll.resize(m_coll.size() - 1);
		}
	}

	void push(T &obj)
	{ m_coll.push_back(obj); }

	void clear()
	{ m_coll.clear(); }
protected:
	std::vector<T> m_coll;
};

#endif /* RECYCLER_H_ */
