// $Id: DecodeGraph.h 2451 2009-08-04 13:46:24Z hieuhoang1972 $
// vim:tabstop=2

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

#include <list>
#include <iterator>

namespace Moses
{

class DecodeStep;

//! list of DecodeStep s which factorizes the translation
class DecodeGraph
{
protected:
	std::list<const DecodeStep*> m_steps;
	size_t m_maxDefaultChartSpan, m_maxSourceSyntaxChartSpan, m_maxTargetSyntaxChartSpan;

public:
	//! iterators
	typedef std::list<const DecodeStep*>::iterator iterator;
	typedef std::list<const DecodeStep*>::const_iterator const_iterator;
	const_iterator begin() const { return m_steps.begin(); }
	const_iterator end() const { return m_steps.end(); }
	
	DecodeGraph(size_t maxDefaultChartSpan, size_t maxSourceSyntaxChartSpan, size_t maxTargetSyntaxChartSpan)
		:m_maxDefaultChartSpan(maxDefaultChartSpan)
		,m_maxSourceSyntaxChartSpan(maxSourceSyntaxChartSpan)
		,m_maxTargetSyntaxChartSpan(maxTargetSyntaxChartSpan)
	{}

	~DecodeGraph();

	size_t GetSize() const
	{ return m_steps.size(); }
	size_t GetMaxDefaultChartSpan() const
	{ return m_maxDefaultChartSpan; }
	size_t GetMaxSourceSyntaxChartSpan() const
	{ return m_maxSourceSyntaxChartSpan; }
	size_t GetMaxTargetSyntaxChartSpan() const
	{ return m_maxTargetSyntaxChartSpan; }

	//! Add another decode step to the graph
	void Add(const DecodeStep *decodeStep)
	{
		m_steps.push_back(decodeStep);
	}
};


}
