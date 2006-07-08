
#pragma once

#include "TypeDef.h"

class  DecodeStep 
{
protected:
	DecodeType m_decodeType;
	int m_id;
		// 2nd = pointer to a phraseDictionary or generationDictionary
public:
	DecodeStep(DecodeType decodeType, int id)
	:m_decodeType(decodeType)
	,m_id(id)
	{
	}
	DecodeType GetDecodeType() const
	{
		return m_decodeType;
	}
	int GetId() const
	{
		return m_id;
	}
	
};
