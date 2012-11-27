#pragma once

#include <string>
#include <vector>

class CollateNode
{
protected:
	std::string m_word;
	std::vector<const CollateNode*> m_equiv;

public:
	explicit CollateNode(const std::string &word)
		:m_word(word)
	{}

	void AddEquivWordNode(const CollateNode &node)
	{
		m_equiv.push_back(&node);
	}
		
	const std::vector<const CollateNode*> &GetEquivNodes() const
	{
		return m_equiv;
	}

	const std::string &GetString() const
	{
		return m_word;
	}

	inline bool operator<(const CollateNode &compare) const
	{
		return m_word < compare.m_word;
	}

};

