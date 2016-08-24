/*
 * NBests.h
 *
 *  Created on: 24 Aug 2016
 *      Author: hieu
 */

#pragma once
#include <boost/unordered_set.hpp>
#include "NBest.h"

namespace Moses2
{
namespace SCFG
{

class NBests
{
public:
	Contenders contenders;
	boost::unordered_set<size_t> distinctHypos;

	virtual ~NBests();

	size_t GetSize() const
	{ return m_coll.size(); }

	const NBest &Get(size_t ind) const
	{ return *m_coll[ind]; }

	void Add(const NBest *nbest)
	{
		m_coll.push_back(nbest);
	}
protected:
	std::vector<const NBest*> m_coll;
};


}
}

