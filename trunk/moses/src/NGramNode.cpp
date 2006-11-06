
#include "NGramNode.h"
#include "NGramCollection.h"

const NGramNode *NGramNode::GetNGram(const Factor *factor) const
{
	return m_map->GetNGram(factor);
}
NGramNode *NGramNode::GetNGram(const Factor *factor)
{
	return m_map->GetNGram(factor);
}
