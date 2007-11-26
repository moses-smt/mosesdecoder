// $Id: Sentence.cpp 1465 2007-09-27 14:16:28Z hieuhoang1972 $
// vim:tabstop=2

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

#include "XmlOption.h"
#include <vector>
#include <string>
#include <iostream>
#include "Util.h"
#include "StaticData.h"

namespace {

std::string ParseXmlTagAttribute(const std::string& tag,const std::string& attributeName){
	/*TODO deal with unescaping \"*/
	string tagOpen = attributeName + "=\"";
	size_t contentsStart = tag.find(tagOpen);
	if (contentsStart == std::string::npos) return "";
	contentsStart += tagOpen.size();
	size_t contentsEnd = tag.find_first_of('"',contentsStart+1);
	if (contentsEnd == std::string::npos) {
		TRACE_ERR("Malformed XML attribute: "<< tag);
		return "";	
	}
	size_t possibleEnd;
	while (tag.at(contentsEnd-1) == '\\' && (possibleEnd = tag.find_first_of('"',contentsEnd+1)) != std::string::npos) {
		contentsEnd = possibleEnd;
	}
	return tag.substr(contentsStart,contentsEnd-contentsStart);
}

std::string TrimXml(const std::string& str) {
	if (str.size() < 2) return str;
	if (str[0] == '<' && str[str.size() - 1] == '>') {
		return str.substr(1, str.size() - 2);
	} else { return str; }
}

bool isXmlTag(const std::string& tag)
{
	return tag[0] == '<';
}

inline std::vector<std::string> TokenizeXml(const std::string& str)
{
	std::string lbrack = "<";
	std::string rbrack = ">";
	std::vector<std::string> tokens;
	// Find first "non-delimiter".
	std::string::size_type cpos = 0;
	std::string::size_type lpos = 0;
	std::string::size_type rpos = 0;

	while (cpos != str.size()) {
  	lpos = str.find_first_of(lbrack, cpos);
		if (lpos != std::string::npos) {
			rpos = str.find_first_of(rbrack, lpos);
			if (rpos == std::string::npos) {
				TRACE_ERR("ERROR: malformed XML: " << str << endl);
				return tokens;
			}
		} else {
			tokens.push_back(str.substr(cpos));
			break;
		}
		if (lpos - cpos > 0)
			tokens.push_back(str.substr(cpos, lpos - cpos));
		tokens.push_back(str.substr(lpos, rpos-lpos+1));
		cpos = rpos + 1;
	}
	return tokens;
}

}

std::vector<XmlOption> ProcessAndStripXMLTags(std::string& line) {
	//parse XML markup in translation line
	std::vector<XmlOption> res;
	std::string rstr;
	if (line.find_first_of('<') == std::string::npos) { return res; }
	std::vector<std::string> xmlTokens = TokenizeXml(line);
	std::string tagName = "";
	std::string tagContents = "";
	std::vector<std::string> altTexts;
	std::vector<std::string> altProbs;
	size_t tagStart=0;
	size_t tagEnd=0;
	size_t curWord=0;
	int numUnary = 0;
	bool doClose = false;
	for (size_t xmlTokenPos = 0 ; xmlTokenPos < xmlTokens.size() ; xmlTokenPos++)
	{
		if(!isXmlTag(xmlTokens[xmlTokenPos]))
		{
			//phrase, not tag
			rstr += xmlTokens[xmlTokenPos];
			curWord = Tokenize(rstr).size();
		}
		else
		{
			//tag data
			std::string tag =  Trim(TrimXml(xmlTokens[xmlTokenPos]));
			VERBOSE(3,"XML TAG IS: " << tag << std::endl);
			std::string::size_type endOfName = tag.find_first_of(' ');
			std::string nextTagName = tag;
			bool isUnary = tag[tag.size() - 1] == '/';
			bool isOpen = tag[1] != '/';
			if (endOfName != std::string::npos) {
				nextTagName = tag.substr(0,endOfName);
				tagContents = tag.substr(endOfName+1);
			}
			if (isOpen)
			{
				//this is an open tag
				tagName = nextTagName;
				altTexts = TokenizeMultiCharSeparator(ParseXmlTagAttribute(tagContents,"english"), "||");
				altProbs = TokenizeMultiCharSeparator(ParseXmlTagAttribute(tagContents,"prob"), "||");
				std::string span = ParseXmlTagAttribute(tagContents,"span");
				tagStart = curWord;
				if (isUnary) {
					numUnary++;
					if (span.empty()) {
						TRACE_ERR("ERROR: unary tags must have a span attribute: " << line << endl);
						return res;
					}
					std::vector<std::string> ij = Tokenize(span, ",");
					if (ij.size() != 2) {
						TRACE_ERR("ERROR: span tag must be of the form \"i,j\": " << line << endl);
						return res;
					}
					tagStart = atoi(ij[0].c_str());
					tagEnd = atoi(ij[1].c_str());
					if (tagEnd < tagStart) {
						TRACE_ERR("ERROR: span tag " << span << " invalid" << endl);
						return res;
					}
					doClose = true;
					VERBOSE(3,"XML TAG IS UNARY" << endl);
				}
				VERBOSE(3,"XML TAG NAME IS: '" << tagName << "'" << endl);
				VERBOSE(3,"XML TAG ENGLISH IS: '" << altTexts[0] << "'" << endl);
				VERBOSE(3,"XML TAG PROB IS: '" << altProbs[0] << "'" << endl);
				VERBOSE(3,"XML TAG STARTS AT WORD: " << tagStart << endl);					
				if (altTexts.size() != altProbs.size()) {
					TRACE_ERR("ERROR: Unequal number of probabilities and translation alternatives: " << line << endl);
					return res;
				}
			}
			else if ((nextTagName.size() == 0) || (nextTagName.at(0) != '/') || (nextTagName.substr(1) != tagName)) 
			{
				//mismatched tag, abort!
				TRACE_ERR("ERROR: tried to parse malformed XML with xml-input enabled: " << line << endl);
				return res;
			}
			else {
				doClose = true;
				tagEnd = curWord-1; //size is inclusive
			}
			if (doClose) {
				VERBOSE(3,"XML END TAG IS: " << nextTagName.substr(1) << endl);
				VERBOSE(3,"XML TAG ENDS AT WORD: " << tagEnd << endl);
				//store translation options into members

				//TODO: deal with multiple XML options here

				if (StaticData::Instance().GetXmlInputType() != XmlIgnore) {
					for (size_t i=0; i<altTexts.size(); ++i) {
						//only store options if we aren't ignoring them
						//set default probability
						float probValue = 1;
						if (altProbs[i] != "") probValue = Scan<float>(altProbs[i]);
						//Convert from prob to log-prob
						float scoreValue = FloorScore(TransformScore(probValue));
						XmlOption option(tagStart,tagEnd,altTexts[i],scoreValue);
						res.push_back(option);
					}
				}
				tagName= "";
				tagContents = "";
				altTexts.clear();
				altProbs.clear();
				doClose = false;
			}
		}
	}
	line = rstr;
	return res;
}

