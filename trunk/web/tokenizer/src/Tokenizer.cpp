// $Id: FactorCollection.cpp 1218 2007-02-16 18:08:37Z hieuhoang1972 $

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

#include "Tokenizer.h"
#include "Util.h"

using namespace std;

bool Tokenizer::m_initialized = false;
std::set<std::string> Tokenizer::m_prefixes;
std::set<std::string> Tokenizer::m_punctuation;
std::set<std::string> Tokenizer::m_quotes;

void Tokenizer::SentenceSeparator(vector<string> &newTokens, const string &token)
{
	string lastChar = token.substr(token.size()-1, 1);
	string word = token.substr(0, token.size()-1);

	if (token.size() == 1 && m_punctuation.find(lastChar) != m_punctuation.end())
	{
		newTokens.push_back(lastChar);
		newTokens.push_back("\n");
	}
	else if (lastChar == ".")
	{
		set<std::string>::iterator iterSet = m_prefixes.find(token);
		if (iterSet != m_prefixes.end())
		{ // found a prefix. add as is
			newTokens.push_back(token);
		}
		else
		{ // a full stop. new sentence
			newTokens.push_back(word);
			newTokens.push_back(lastChar);
			newTokens.push_back("\n");
		}
	}
	else if (m_punctuation.find(lastChar) != m_punctuation.end())
	{
		newTokens.push_back(word);
		newTokens.push_back(lastChar);
		newTokens.push_back("\n");
	}

	else
	{ // just a normal word
		newTokens.push_back(token);
	}
}

void Tokenizer::QuotesFirst(vector<string> &newTokens, const string &token)
{
	string lastChar = token.substr(0, 1);
	string word = token.substr(1, token.size()-1);
	if (m_quotes.find(lastChar) != m_quotes.end())
	{
		newTokens.push_back(lastChar);
		if (word != "")
			newTokens.push_back(word);
	}
	else
	{
		newTokens.push_back(token);
	}
}

void Tokenizer::QuotesLast(vector<string> &newTokens, const string &token)
{
	string lastChar = token.substr(token.size()-1, 1);
	string word = token.substr(0, token.size()-1);
	if (m_quotes.find(lastChar) != m_quotes.end())
	{
		if (word != "")
			newTokens.push_back(word);
		newTokens.push_back(lastChar);
	}
	else
	{
		newTokens.push_back(token);
	}
}

string Tokenizer::Tokenize(const string &input)
{
	stringstream buffer("");
	vector<string> newTokens
								,oldTokens = ::Tokenize(input, " \t\n");

	vector<string>::iterator iterTokens;
	for (iterTokens = oldTokens.begin() ; iterTokens != oldTokens.end() ; ++iterTokens)
	{
		string &token = *iterTokens;
		SentenceSeparator(newTokens, token);
	}
	oldTokens = newTokens;

	newTokens.clear();
	for (iterTokens = oldTokens.begin() ; iterTokens != oldTokens.end() ; ++iterTokens)
	{
		string &token = *iterTokens;
		QuotesFirst(newTokens, token);
	}
	oldTokens = newTokens;
	
	newTokens.clear();
	for (iterTokens = oldTokens.begin() ; iterTokens != oldTokens.end() ; ++iterTokens)
	{
		string &token = *iterTokens;
		QuotesLast(newTokens, token);
	}

	return Join(" ", newTokens);
}


Tokenizer::Tokenizer(const std::string &language)
	:m_language(language)
{
	if (m_initialized) 
		return;
	
	m_initialized = true;

	m_prefixes.insert("adj.");
	m_prefixes.insert("adm.");
	m_prefixes.insert("adv.");
	m_prefixes.insert("asst.");
	m_prefixes.insert("ave.");
	m_prefixes.insert("bldg.");
	m_prefixes.insert("brig.");
	m_prefixes.insert("bros.");
	m_prefixes.insert("capt.");
	m_prefixes.insert("cmdr.");
	m_prefixes.insert("col.");
	m_prefixes.insert("comdr.");
	m_prefixes.insert("con.");
	m_prefixes.insert("corp.");
	m_prefixes.insert("cpl.");
	m_prefixes.insert("dr.");
	m_prefixes.insert("ens.");
	m_prefixes.insert("gen.");
	m_prefixes.insert("gov.");
	m_prefixes.insert("hon.");
	m_prefixes.insert("hosp.");
	m_prefixes.insert("insp.");
	m_prefixes.insert("lt.");
	m_prefixes.insert("maj.");
	m_prefixes.insert("messrs.");
	m_prefixes.insert("mlle.");
	m_prefixes.insert("mme.");
	m_prefixes.insert("mr.");
	m_prefixes.insert("mrs.");
	m_prefixes.insert("ms.");
	m_prefixes.insert("msgr.");
	m_prefixes.insert("op.");
	m_prefixes.insert("ord.");
	m_prefixes.insert("pfc.");
	m_prefixes.insert("ph.");
	m_prefixes.insert("prof.");
	m_prefixes.insert("pvt.");
	m_prefixes.insert("rep.");
	m_prefixes.insert("reps.");
	m_prefixes.insert("res.");
	m_prefixes.insert("rev.");
	m_prefixes.insert("rt.");
	m_prefixes.insert("sen.");
	m_prefixes.insert("sens.");
	m_prefixes.insert("sgt.");
	m_prefixes.insert("sr.");
	m_prefixes.insert("st.");
	m_prefixes.insert("supt.");
	m_prefixes.insert("surg.");
	m_prefixes.insert("v.");
	m_prefixes.insert("vs.");

	m_punctuation.insert(":");
	m_punctuation.insert(".");
	m_punctuation.insert("!");
	m_punctuation.insert("?");
	m_punctuation.insert(";");

	m_quotes.insert("\"");
	m_quotes.insert("'");
	m_quotes.insert("�");
	m_quotes.insert("�");
}
