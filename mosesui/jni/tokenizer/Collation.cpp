
#include <cassert>
#include "Collation.h"
#include "../moses/src/InputFileStream.h"
#include "../moses/src/Util.h"
#include "Tokenizer.h"

using namespace std;
using namespace Moses;

bool Collation::LoadCollation(const std::string &collateLetterPath, const std::string &sourceLMPath)
{	
	// load letter
	InputFileStream 	letterFile(collateLetterPath);

	string line;
	int lineNo = 0;

	while( !getline(letterFile, line, '\n').eof())
	{
		lineNo++;
		vector<string> data = ::Tokenize(line);
		assert(data.size() == 2);

		m_collateChar[data[0]] = data[1];
	}

	// load words
	InputFileStream wordFile(sourceLMPath);

	while( !getline(wordFile, line, '\n').eof())
	{
		vector<string> tokens = ::Tokenize(line, "\t");

		if (tokens.size() >= 2)
		{ 
			// only get unigram
			// split unigram/bigram trigrams
			vector<string> factorStr = ::Tokenize(tokens[1], " ");
			if (factorStr.size() > 1)
			{
				break;
			}
			if (factorStr.size() == 1)
			{
				const string &word = factorStr[0];
				string collated = GetCollateEquiv(word);
				
				CollateNode searchWordNode(word)
										,searchCollateNode(collated);

				pair< std::set<CollateNode>::iterator, bool> ret;
				ret = m_collateWord.insert(searchWordNode);
				const CollateNode &wordNode		= *ret.first;

				ret = m_collateWord.insert(searchCollateNode);
				
				// TODO hack for gcc
				CollateNode &collateNode		= const_cast<CollateNode&>(*ret.first);

				collateNode.AddEquivWordNode(wordNode);
			}
		}
	}

	return true;
}

// string find-and-replace
string FindAndReplace
(const string& source, const string target, const string &replacement)
{
	string str = source;
	string::size_type pos = 0,   // where we are now
										found;     // where the found data is

	if (target.size () > 0)   // searching for nothing will cause a loop
  {
		while ((found = str.find (target, pos)) != string::npos)
		{
			str.replace (found, target.size (), replacement);
			pos = found + replacement.size ();
		}
	}

	return str;
};   // end of FindAndReplace


string Collation::GetCollateEquiv(const string &input) const
{
	string output = input;


	map<string, string>::const_iterator iter;
	for (iter = m_collateChar.begin(); iter != m_collateChar.end(); ++iter)
	{
		const std::string &letter = iter->first
											, equiv_letter = iter->second;
		output = FindAndReplace(output, letter, equiv_letter);
	}

	return output;
}

const CollateNode *Collation::GetCollateEquivNode(const std::string &input) const
{
	string equivWord = GetCollateEquiv(input);
	
	std::set<CollateNode>::const_iterator iter;
	iter = m_collateWord.find(CollateNode(equivWord));

	if (iter == m_collateWord.end())
	{ // not found
		return NULL;
	}
	else
	{
		const CollateNode &node = *iter;
		return &node;
	}
}

