// $Id: IOCommandLine.h 906 2006-10-21 16:31:45Z hieuhoang1972 $

/***********************************************************************
Copyright (c) 2007 Hieu Hoang
All rights reserved.
***********************************************************************/

#pragma once

#include <string>
#include <vector>
#include <set>
#include "Collation.h"
#include "../moses/src/TypeDef.h"

class Tokenizer
{
protected:
	std::string m_language;
	Collation m_collation;

	static bool m_initialized;
	static std::set<std::string> m_prefixes;
	static std::set<std::string> m_punctuation;
	static std::set<std::string> m_quotes;

	void FormatConfusionNetwork(const std::vector<std::string> &input
														,std::vector<std::string> &output);

public:

	Tokenizer();
	void SetLanguage(const std::string &language);
	bool LoadCollation(const std::string &collateLetterPath, const std::string &sourceLMPath);
	std::vector<std::string> Tokenize(const std::string &input, Moses::InputTypeEnum inputType);
	void SentenceSeparator(std::vector<std::string> &newTokens, const std::string &token);
	void QuotesFirst(std::vector<std::string> &newTokens, const std::string &token);
	void QuotesLast(std::vector<std::string> &newTokens, const std::string &token);
};

