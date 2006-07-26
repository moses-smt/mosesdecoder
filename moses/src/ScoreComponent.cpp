// $Id$

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

#include "TypeDef.h"
#include "ScoreComponent.h"
#include "Dictionary.h"

ScoreComponent::ScoreComponent()
:m_dictionary(NULL)
{ // used by collection
}

ScoreComponent::ScoreComponent(const Dictionary *dictionary)
	:m_dictionary(dictionary)
	,m_scoreComponent(dictionary->GetNumScoreComponents())
{
}

ScoreComponent::ScoreComponent(const ScoreComponent &copy)
:m_dictionary(copy.m_dictionary)
{
	if (m_dictionary != NULL)
	{
		const size_t noScoreComponent = GetNumScoreComponents();
		for (size_t i = 0 ; i < noScoreComponent ; i++)
		{
			m_scoreComponent.push_back(copy[i]);
		}
	}
}

size_t ScoreComponent::GetNumScoreComponents() const
{
	if (m_dictionary != NULL)
		return m_dictionary->GetNumScoreComponents();
	else
		return 0;
}

void ScoreComponent::Reset()
{
	if (m_dictionary != NULL)
	{
		const size_t noScoreComponent = GetNumScoreComponents();
		assert(noScoreComponent<=m_scoreComponent.size());
		std::fill(m_scoreComponent.begin(),m_scoreComponent.begin()+noScoreComponent,0.0);
	}
}

std::ostream& operator<<(std::ostream &out, const ScoreComponent &scoreComponent)
{
	const size_t noScoreComponent = scoreComponent.GetNumScoreComponents();
	if (noScoreComponent > 0)
	{
		out << scoreComponent[0];
		for (size_t i = 1 ; i < noScoreComponent ; i++)
		{
			out << "," << scoreComponent[i];
		}
	}
	return out;
}

