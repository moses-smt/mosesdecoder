/*
 * Recycler.h
 *
 *  Created on: 5 Nov 2015
 *      Author: hieu
 */

#ifndef RECYCLER_H_
#define RECYCLER_H_

#include <queue>

template<typename T>
class Recycler {
public:
	Recycler()
	{

	}
	virtual ~Recycler()
	{}

	bool empty() const
	{ return m_coll.empty(); }

	T &front()
	{ return m_coll.front(); }

	void pop()
	{ m_coll.pop(); }

	void push(T &obj)
	{ m_coll.push(obj); }

protected:
	std::queue<T> m_coll;
};

#endif /* RECYCLER_H_ */
