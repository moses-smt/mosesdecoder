#include "SyntaxTree.h"

void SyntaxTree::Add(int startPos, int endPos, const std::string &label)
{
	Range range(startPos, endPos);
	Labels &labels = m_coll[range];
	labels.push_back(label);
}

