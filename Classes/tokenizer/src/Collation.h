
#pragma once

#include <map>
#include <set>
#include "CollateNode.h"

class Collation
{
protected:
	std::map<std::string, std::string> m_collateChar;
	std::set<CollateNode> m_collateWord;

public:
	bool LoadCollation(const std::string &collateLetterPath, const std::string &sourceLMPath);
	std::string GetCollateEquiv(const std::string &input) const;
	const CollateNode *GetCollateEquivNode(const std::string &input) const;

};

