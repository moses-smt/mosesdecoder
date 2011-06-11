// $Id: IOCommandLine.h 906 2006-10-21 16:31:45Z hieuhoang1972 $

/***********************************************************************
Copyright (c) 2007 Hieu Hoang
All rights reserved.
***********************************************************************/

#include <iostream>
#include <algorithm>
#include "Tokenizer.h"
#include "../moses/src/Util.h"
#include "../moses/src/InputFileStream.h"
#include "../moses/src/UserMessage.h"

using namespace std;
using namespace Moses;

bool Tokenizer::m_initialized = false;
std::set<std::string> Tokenizer::m_prefixes;
std::set<std::string> Tokenizer::m_punctuation;
std::set<std::string> Tokenizer::m_quotes;

std::string JoinMe(const std::string& delimiter, const std::vector<string>::iterator &b, const std::vector<string>::iterator &e)
{
  std::ostringstream outstr;
  std::vector<string>::const_iterator iter = b;

  if (iter != e)
  {
    outstr << *iter;
    ++iter;
    for( ; iter != e; ++iter)
      {
	const string &item = *iter;
	outstr << delimiter << item;

      }
  }
  return outstr.str();
}

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

vector<string> Tokenizer::Tokenize(const string &input, InputTypeEnum inputType)
{
	stringstream buffer("");
	vector<string> newTokens
								,oldTokens = ::Tokenize(input, " \t\n"); // dumb 1st pass - separate using space/tab/newline

	// separate sentences. vector then contains line \n char
	vector<string>::iterator iterTokens;
	for (iterTokens = oldTokens.begin() ; iterTokens != oldTokens.end() ; ++iterTokens)
	{
		string &token = *iterTokens;
		SentenceSeparator(newTokens, token);
	}

	// edge case - last word isn't a .
	if (newTokens.back() != "\n")
		newTokens.push_back("\n");

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

	if (m_language == "fr")
	{ // t'aime --> t' aime
		oldTokens = newTokens;
		
		newTokens.clear();
		for (iterTokens = oldTokens.begin() ; iterTokens != oldTokens.end() ; ++iterTokens)
		{
			string &token = *iterTokens;
			size_t pos = token.find('\'');
			if (pos == token.npos)
			{
				newTokens.push_back(token);
			}
			else
			{
				newTokens.push_back(token.substr(0, pos + 1));
				
				string secondToken = token.substr(pos+1, token.size() - pos - 1);
				newTokens.push_back(secondToken);
			}
		}
	}
	else
	{ // I've --> I 've
		oldTokens = newTokens;
		
		newTokens.clear();
		for (iterTokens = oldTokens.begin() ; iterTokens != oldTokens.end() ; ++iterTokens)
		{
			string &token = *iterTokens;
			size_t pos = token.find('\'');
			if (pos == token.npos)
			{
				newTokens.push_back(token);
			}
			else
			{
				string firstToken = token.substr(0, pos);
				newTokens.push_back(firstToken);
				
				string secondToken = token.substr(pos, token.size() - pos);
				newTokens.push_back(secondToken);
			}
		}
	}
	
	vector<string> ret;
	if (inputType == SentenceInput)
	{ // put words back into string, 1 entry per sentence
		vector<string>::iterator startIter = newTokens.begin();
		vector<string>::iterator endIter = std::find(startIter, newTokens.end(), "\n");
		while (endIter != newTokens.end())
		{
			string str = JoinMe(" ", startIter, endIter);
			ret.push_back(str);

			startIter = endIter + 1;
			endIter = std::find(startIter, newTokens.end(), "\n");
		}
	}
	else if (inputType == ConfusionNetworkInput)
	{
		FormatConfusionNetwork(newTokens, ret);
	}
	else
	{
		UserMessage::Add("Unknown input type. Aborting");
		abort();
	}

	return ret;
}

void Tokenizer::FormatConfusionNetwork(const vector<string> &input, vector<string> &output)
{
	vector<string>::const_iterator iter;

	stringstream strStrme("");

	for (iter = input.begin() ; iter != input.end() ; ++iter)
	{
		const string &origWord = *iter;

		if (origWord == "\n")
		{ // finished with sentence
			string cnWord = Trim(strStrme.str());
			output.push_back(cnWord +  "\n");
			strStrme.str("");
			continue;
		}

		const CollateNode *node = m_collation.GetCollateEquivNode(origWord);
	
		if (node == NULL)
		{ // unknown word. add as is
			strStrme << origWord << " 1.0\n";
		}
		else
		{ // found equiv collated words
			const std::vector<const CollateNode*> &nodeColl = node->GetEquivNodes();
			
			vector<const CollateNode*>::const_iterator iterNode;
			for (iterNode = nodeColl.begin() ; iterNode != nodeColl.end() ; ++iterNode)
			{
				const CollateNode &equivNode = **iterNode;
				strStrme << equivNode.GetString() << " 1.0 ";
			}
			strStrme << endl;
		}
	}
}

void Tokenizer::SetLanguage(const string &language)
{
	m_language = language;
}

Tokenizer::Tokenizer()
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
	m_quotes.insert("ì");
	m_quotes.insert("Ñ");
	m_quotes.insert(",");
}

bool Tokenizer::LoadCollation(const std::string &collateLetterPath, const std::string &sourceLMPath)
{
	return m_collation.LoadCollation(collateLetterPath, sourceLMPath);
}

