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


std::vector<XmlOption> parseXMLOptions(const std::string& line) {
  //parse XML markup in translation line
  std::vector<XmlOption> res;
  if (line.find_first_of('<') == std::string::npos) { return res; }
  std::vector<string> xmlTokens = Tokenize(line,"<>");
  std::string tagName = "";
  std::string tagContents = "";
  std::vector<std::string> altTexts;
  std::vector<std::string> altProbs;
  size_t offset=0;
  size_t tagStart=0;
  if (xmlTokens.size()>1 && line.at(0) == '<') offset=1;
  for (size_t xmlTokenPos = 0 ; xmlTokenPos < xmlTokens.size() ; xmlTokenPos++)
  {
    if(((xmlTokenPos+offset) % 2) == 0)
    {
      //phrase, not tag
      Phrase::CreateFromString(factorOrder,xmlTokens[xmlTokenPos],factorDelimiter);
    }
    else
    {
      //TODO: support UNARY tags

      //tag data
      std::string tag =  Trim(xmlTokens[xmlTokenPos]);
      VERBOSE(3,"XML TAG IS: " << tag << endl);
      std::string::size_type endOfName = xmlTokens[xmlTokenPos].find_first_of(' ');
      std::string nextTagName = tag;
      if (endOfName != std::string::npos) {
        nextTagName = xmlTokens[xmlTokenPos].substr(0,endOfName);
        tagContents = xmlTokens[xmlTokenPos].substr(endOfName+1);
      }
      if ((xmlTokenPos-1+offset) % 4 == 0)
      {
        //this is an open tag
        tagName = nextTagName;
        altTexts = Tokenize(ParseXmlTagAttribute(tagContents,"english"), "|");
        altProbs = Tokenize(ParseXmlTagAttribute(tagContents,"prob"), "|");
        tagStart =  Phrase::GetSize();
        VERBOSE(3,"XML TAG NAME IS: '" << tagName << "'" << endl);
        VERBOSE(3,"XML TAG ENGLISH IS: '" << altTexts[0] << "'" << endl);
        VERBOSE(3,"XML TAG PROB IS: '" << altProbs[0] << "'" << endl);
        VERBOSE(3,"XML TAG STARTS AT WORD: " << Phrase::GetSize() << endl);					
        if (altTexts.size() != altProbs.size()) {
          TRACE_ERR("ERROR: Unequal number of probabilities and translation alternatives: " << line << endl);
          return 0;
        }
      }
      else if ((nextTagName.size() == 0) || (nextTagName.at(0) != '/') || (nextTagName.substr(1) != tagName)) 
      {
        //mismatched tag, abort!
        TRACE_ERR("ERROR: tried to parse malformed XML with xml-input enabled: " << line << endl);
        return 0;
      }
      else 
      {
        VERBOSE(3,"XML END TAG IS: " << nextTagName.substr(1) << endl);
        VERBOSE(3,"XML TAG ENDS AT WORD: " << Phrase::GetSize() << endl);
        //store translation options into members
        size_t tagEnd = Phrase::GetSize()-1; //size is inclusive

        //TODO: deal with multiple XML options here

        if (staticData.GetXmlInputType() != XmlIgnore) {
          for (size_t i=0; i<altTexts.size(); ++i) {
            //only store options if we aren't ignoring them
            //set default probability
            float probValue = 1;
            if (altProbs[i] != "") probValue = Scan<float>(altProbs[i]);
            //Convert from prob to log-prob
            float scoreValue = FloorScore(TransformScore(probValue));
            XmlOption option(tagStart,tagEnd,altTexts[i],scoreValue);
            m_xmlOptionsList.push_back(option);
          }
        }
        tagName= "";
        tagContents = "";
        altTexts.clear();
        altProbs.clear();
      }
    }
  }
}

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
