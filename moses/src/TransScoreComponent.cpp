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
#include "Util.h"
#include "TransScoreComponent.h"
#include "PhraseDictionary.h"

TransScoreComponent::TransScoreComponent(const PhraseDictionary *phraseDictionary)
:	m_phraseDictionary(phraseDictionary)
{
	m_scoreComponent = (float*) malloc(sizeof(float) * phraseDictionary->GetNoScoreComponent());
}

TransScoreComponent::TransScoreComponent(const TransScoreComponent &copy)
	:m_phraseDictionary(copy.m_phraseDictionary)
{
	const size_t noScoreComponent = m_phraseDictionary->GetNoScoreComponent();
	m_scoreComponent = (float*) malloc(sizeof(float) * noScoreComponent);
	
	for (size_t i = 0 ; i < noScoreComponent ; i++)
	{
		m_scoreComponent[i] = copy[i];
	}
}

TransScoreComponent::~TransScoreComponent()
{
	free(m_scoreComponent);
}

void TransScoreComponent::Reset()
{
	const size_t noScoreComponent = m_phraseDictionary->GetNoScoreComponent();
	for (size_t i = 0 ; i < noScoreComponent ; i++)
	{
		m_scoreComponent[i] = 0;
	}
}

size_t TransScoreComponent::GetNoScoreComponent() const
{
	return m_phraseDictionary->GetNoScoreComponent();
}

std::ostream& operator<<(std::ostream &out, const TransScoreComponent &transScoreComponent)
{
	const size_t noScoreComponent = transScoreComponent.GetNoScoreComponent();
	out << transScoreComponent[0];
	for (size_t i = 1 ; i < noScoreComponent ; i++)
	{
		out << "," << transScoreComponent[i];
	}	
	return out;
}

