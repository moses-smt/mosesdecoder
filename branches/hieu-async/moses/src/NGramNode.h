// $Id: NGramNode.h,v 1.2 2006/06/14 18:27:55 h.hoang Exp $

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

#include "Factor.h"

class NGramCollection;

class NGramNode
{
protected:
	float						m_score, m_logBackOff;
	NGramCollection	*m_map;
	NGramNode				*m_rootNGram;
public:
	NGramNode();
	~NGramNode();
	NGramCollection *GetNGramColl()
	{
		return m_map;
	}

	const NGramNode *GetNGram(const Factor *factor) const;
	NGramNode *GetNGram(const Factor *factor);

	const NGramNode *GetRootNGram() const
	{
		return m_rootNGram;
	}
	void SetRootNGram(NGramNode *rootNGram)
	{
		m_rootNGram = rootNGram;
	}

	float GetScore() const
	{
		return m_score;
	}
	float GetLogBackOff() const
	{
		return m_logBackOff;
	}
	void SetScore(float score)
	{
		m_score = score;
	}
	void SetLogBackOff(float logBackOff)
	{
		m_logBackOff = logBackOff;
	}

};

