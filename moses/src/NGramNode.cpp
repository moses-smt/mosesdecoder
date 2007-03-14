
#include "NGramNode.h"
#include "NGramCollection.h"

NGramNode::NGramNode()
{
	m_map = new NGramCollection();
}
NGramNode::~NGramNode()
{
	delete m_map;
}

const NGramNode *NGramNode::GetNGram(const Factor *factor) const
{
	return m_map->GetNGram(factor);
}
NGramNode *NGramNode::GetNGram(const Factor *factor)
{
	return m_map->GetNGram(factor);
}
