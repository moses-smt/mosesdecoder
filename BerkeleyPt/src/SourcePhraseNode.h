#pragma once
/*
 *  SourcePhraseNode.h
 *  BerkeleyPt
 *
 *  Created by Hieu Hoang on 03/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "SourcePhraseNode.h"
#include "Vocab.h"

namespace MosesBerkeleyPt
{
	

class SourcePhraseNode
{
protected:
	VocabId m_vocabId;
	
public:
	SourcePhraseNode(const SourcePhraseNode &copy)
	:m_vocabId(copy.m_vocabId)
	{}
};
	
	
};

