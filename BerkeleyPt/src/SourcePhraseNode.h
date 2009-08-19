#pragma once
/*
 *  SourcePhraseNode.h
 *  BerkeleyPt
 *
 *  Created by Hieu Hoang on 03/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <iostream>
#include "SourcePhraseNode.h"
#include "Vocab.h"
#include "../../moses/src/TypeDef.h"

namespace MosesBerkeleyPt
{
	
class SourcePhraseNode
{
	friend std::ostream& operator<<(std::ostream &out, const SourcePhraseNode &node);

protected:
	Moses::UINT32 m_sourceNodeId;
	
public:
	SourcePhraseNode(const SourcePhraseNode &copy)
	:m_sourceNodeId(copy.m_sourceNodeId)
	{}
	SourcePhraseNode(long sourceNodeId)
	:m_sourceNodeId(sourceNodeId)
	{}

	Moses::UINT32 GetSourceNodeId() const
	{ return m_sourceNodeId; }
};
	
	
};

