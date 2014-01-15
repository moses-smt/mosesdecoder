// $Id$
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
#include "WordsRange.h"
#include "TargetPhrase.h"
#include "ReorderingConstraint.h"
#include "FactorCollection.h"

namespace Moses
{
using namespace std;

string ParseXmlTagAttribute(const string& tag,const string& attributeName)
{
  /*TODO deal with unescaping \"*/
  string tagOpen = attributeName + "=\"";
  size_t contentsStart = tag.find(tagOpen);
  if (contentsStart == string::npos) return "";
  contentsStart += tagOpen.size();
  size_t contentsEnd = tag.find_first_of('"',contentsStart+1);
  if (contentsEnd == string::npos) {
    TRACE_ERR("Malformed XML attribute: "<< tag);
    return "";
  }
  size_t possibleEnd;
  while (tag.at(contentsEnd-1) == '\\' && (possibleEnd = tag.find_first_of('"',contentsEnd+1)) != string::npos) {
    contentsEnd = possibleEnd;
  }
  return tag.substr(contentsStart,contentsEnd-contentsStart);
}

/**
 * Remove "<" and ">" from XML tag
 *
 * \param str xml token to be stripped
 * \param lbrackStr xml tag's left bracket string, typically "<"
 * \param rbrackStr xml tag's right bracket string, typically ">"
 */
string TrimXml(const string& str, const std::string& lbrackStr, const std::string& rbrackStr)
{
  // too short to be xml token -> do nothing
  if (str.size() < lbrackStr.length()+rbrackStr.length() ) return str;

  // strip first and last character
  if (str.substr(0,lbrackStr.length()) == lbrackStr  &&  str.substr(str.size()-rbrackStr.length()) == rbrackStr) {
    return str.substr(lbrackStr.length(), str.size()-lbrackStr.length()-rbrackStr.length());
  }
  // not an xml token -> do nothing
  else {
    return str;
  }
}

/**
 * Check if the token is an XML tag, i.e. starts with "<"
 *
 * \param tag token to be checked
 * \param lbrackStr xml tag's left bracket string, typically "<"
 * \param rbrackStr xml tag's right bracket string, typically ">"
 */
bool isXmlTag(const string& tag, const std::string& lbrackStr, const std::string& rbrackStr)
{
  return (tag.substr(0,lbrackStr.length()) == lbrackStr &&
          (tag[lbrackStr.length()] == '/' ||
           (tag[lbrackStr.length()] >= 'a' && tag[lbrackStr.length()] <= 'z') ||
           (tag[lbrackStr.length()] >= 'A' && tag[lbrackStr.length()] <= 'Z')));
}

/**
 * Split up the input character string into tokens made up of
 * either XML tags or text.
 * example: this <b> is a </b> test .
 *       => (this ), (<b>), ( is a ), (</b>), ( test .)
 *
 * \param str input string
 * \param lbrackStr xml tag's left bracket string, typically "<"
 * \param rbrackStr xml tag's right bracket string, typically ">"
 */
vector<string> TokenizeXml(const string& str, const std::string& lbrackStr, const std::string& rbrackStr)
{
  string lbrack = lbrackStr; // = "<";
  string rbrack = rbrackStr; // = ">";
  vector<string> tokens; // vector of tokens to be returned
  string::size_type cpos = 0; // current position in string
  string::size_type lpos = 0; // left start of xml tag
  string::size_type rpos = 0; // right end of xml tag

  // walk thorugh the string (loop vver cpos)
  while (cpos != str.size()) {
    // find the next opening "<" of an xml tag
    lpos = str.find(lbrack, cpos);			// lpos = str.find_first_of(lbrack, cpos);
    if (lpos != string::npos) {
      // find the end of the xml tag
      rpos = str.find(rbrack, lpos+lbrackStr.length()-1);			// rpos = str.find_first_of(rbrack, lpos);
      // sanity check: there has to be closing ">"
      if (rpos == string::npos) {
        TRACE_ERR("ERROR: malformed XML: " << str << endl);
        return tokens;
      }
    } else { // no more tags found
      // add the rest as token
      tokens.push_back(str.substr(cpos));
      break;
    }

    // add stuff before xml tag as token, if there is any
    if (lpos - cpos > 0)
      tokens.push_back(str.substr(cpos, lpos - cpos));

    // add xml tag as token
    tokens.push_back(str.substr(lpos, rpos-lpos+rbrackStr.length()));
    cpos = rpos + rbrackStr.length();
  }
  return tokens;
}

/**
 * Process a sentence with xml annotation
 * Xml tags may specifiy additional/replacing translation options
 * and reordering constraints
 *
 * \param line in: sentence, out: sentence without the xml
 * \param res vector with translation options specified by xml
 * \param reorderingConstraint reordering constraint zones specified by xml
 * \param walls reordering constraint walls specified by xml
 * \param lbrackStr xml tag's left bracket string, typically "<"
 * \param rbrackStr xml tag's right bracket string, typically ">"
 */
bool ProcessAndStripXMLTags(string &line, vector<XmlOption*> &res, ReorderingConstraint &reorderingConstraint, vector< size_t > &walls,
                            std::vector< std::pair<size_t, std::string> > &placeholders,
                            int offset,
                            const std::string& lbrackStr, const std::string& rbrackStr)
{
  //parse XML markup in translation line

  const StaticData &staticData = StaticData::Instance();

  // no xml tag? we're done.
//if (line.find_first_of('<') == string::npos) {
  if (line.find(lbrackStr) == string::npos) {
    return true;
  }

  // break up input into a vector of xml tags and text
  // example: (this), (<b>), (is a), (</b>), (test .)
  vector<string> xmlTokens = TokenizeXml(line, lbrackStr, rbrackStr);

  // we need to store opened tags, until they are closed
  // tags are stored as tripled (tagname, startpos, contents)
  typedef pair< string, pair< size_t, string > > OpenedTag;
  vector< OpenedTag > tagStack; // stack that contains active opened tags

  string cleanLine; // return string (text without xml)
  size_t wordPos = 0; // position in sentence (in terms of number of words)

  const vector<FactorType> &outputFactorOrder = staticData.GetOutputFactorOrder();
  const string &factorDelimiter = staticData.GetFactorDelimiter();

  // loop through the tokens
  for (size_t xmlTokenPos = 0 ; xmlTokenPos < xmlTokens.size() ; xmlTokenPos++) {
    // not a xml tag, but regular text (may contain many words)
    if(!isXmlTag(xmlTokens[xmlTokenPos], lbrackStr, rbrackStr)) {
      // add a space at boundary, if necessary
      if (cleanLine.size()>0 &&
          cleanLine[cleanLine.size() - 1] != ' ' &&
          xmlTokens[xmlTokenPos][0] != ' ') {
        cleanLine += " ";
      }
      cleanLine += xmlTokens[xmlTokenPos]; // add to output
      wordPos = Tokenize(cleanLine).size(); // count all the words
    }

    // process xml tag
    else {
      // *** get essential information about tag ***

      // strip extra boundary spaces and "<" and ">"
      string tag =  Trim(TrimXml(xmlTokens[xmlTokenPos], lbrackStr, rbrackStr));
      VERBOSE(3,"XML TAG IS: " << tag << std::endl);

      if (tag.size() == 0) {
        TRACE_ERR("ERROR: empty tag name: " << line << endl);
        return false;
      }

      // check if unary (e.g., "<wall/>")
      bool isUnary = ( tag[tag.size() - 1] == '/' );

      // check if opening tag (e.g. "<a>", not "</a>")g
      bool isClosed = ( tag[0] == '/' );
      bool isOpen = !isClosed;

      if (isClosed && isUnary) {
        TRACE_ERR("ERROR: can't have both closed and unary tag " << lbrackStr << tag << rbrackStr << ": " << line << endl);
        return false;
      }

      if (isClosed)
        tag = tag.substr(1); // remove "/" at the beginning
      if (isUnary)
        tag = tag.substr(0,tag.size()-1); // remove "/" at the end

      // find the tag name and contents
      string::size_type endOfName = tag.find_first_of(' ');
      string tagName = tag;
      string tagContent = "";
      if (endOfName != string::npos) {
        tagName = tag.substr(0,endOfName);
        tagContent = tag.substr(endOfName+1);
      }

      // *** process new tag ***

      if (isOpen || isUnary) {
        // put the tag on the tag stack
        OpenedTag openedTag = make_pair( tagName, make_pair( wordPos, tagContent ) );
        tagStack.push_back( openedTag );
        VERBOSE(3,"XML TAG " << tagName << " (" << tagContent << ") added to stack, now size " << tagStack.size() << endl);
      }

      // *** process completed tag ***

      if (isClosed || isUnary) {
        // pop last opened tag from stack;
        if (tagStack.size() == 0) {
          TRACE_ERR("ERROR: tag " << tagName << " closed, but not opened" << ":" << line << endl);
          return false;
        }
        OpenedTag openedTag = tagStack.back();
        tagStack.pop_back();

        // tag names have to match
        if (openedTag.first != tagName) {
          TRACE_ERR("ERROR: tag " << openedTag.first << " closed by tag " << tagName << ": " << line << endl );
          return false;
        }

        // assemble remaining information about tag
        size_t startPos = openedTag.second.first;
        string tagContent = openedTag.second.second;
        size_t endPos = wordPos;

        // span attribute overwrites position
        string span = ParseXmlTagAttribute(tagContent,"span");
        if (! span.empty()) {
          vector<string> ij = Tokenize(span, "-");
          if (ij.size() != 1 && ij.size() != 2) {
            TRACE_ERR("ERROR: span attribute must be of the form \"i-j\" or \"i\": " << line << endl);
            return false;
          }
          startPos = atoi(ij[0].c_str());
          if (ij.size() == 1) endPos = startPos + 1;
          else endPos = atoi(ij[1].c_str()) + 1;
        }

        VERBOSE(3,"XML TAG " << tagName << " (" << tagContent << ") spanning " << startPos << " to " << (endPos-1) << " complete, commence processing" << endl);

        // special tag: wall
        if (tagName == "wall") {
          size_t start = (startPos == 0) ? 0 : startPos-1;
          for(size_t pos = start; pos < endPos; pos++)
            walls.push_back( pos );
        }

        // special tag: zone
        else if (tagName == "zone") {
          if (startPos >= endPos) {
            TRACE_ERR("ERROR: zone must span at least one word: " << line << endl);
            return false;
          }
          reorderingConstraint.SetZone( startPos, endPos-1 );
        }

        // name-entity placeholder
        else if (tagName == "ne") {
          if (startPos != (endPos - 1)) {
            TRACE_ERR("ERROR: Placeholder must only span 1 word: " << line << endl);
            return false;
          }
          string entity = ParseXmlTagAttribute(tagContent,"entity");
          placeholders.push_back(std::pair<size_t, std::string>(startPos, entity));
        }

        // default: opening tag that specifies translation options
        else {
          if (startPos >= endPos) {
            TRACE_ERR("ERROR: tag " << tagName << " must span at least one word: " << line << endl);
            return false;
          }

          // specified translations -> vector of phrases
          // multiple translations may be specified, separated by "||"
          vector<string> altTexts = TokenizeMultiCharSeparator(ParseXmlTagAttribute(tagContent,"translation"), "||");
          if( altTexts.size() == 1 && altTexts[0] == "" )
            altTexts.pop_back(); // happens when nothing specified
          // deal with legacy annotations: "translation" was called "english"
          vector<string> moreAltTexts = TokenizeMultiCharSeparator(ParseXmlTagAttribute(tagContent,"english"), "||");
          if (moreAltTexts.size()>1 || moreAltTexts[0] != "") {
            for(vector<string>::iterator translation=moreAltTexts.begin();
                translation != moreAltTexts.end();
                translation++) {
              string t = *translation;
              altTexts.push_back( t );
            }
          }

          // specified probabilities for the translations -> vector of probs
          vector<string> altProbs = TokenizeMultiCharSeparator(ParseXmlTagAttribute(tagContent,"prob"), "||");
          if( altProbs.size() == 1 && altProbs[0] == "" )
            altProbs.pop_back(); // happens when nothing specified

          // report what we have processed so far
          VERBOSE(3,"XML TAG NAME IS: '" << tagName << "'" << endl);
          VERBOSE(3,"XML TAG TRANSLATION IS: '" << altTexts[0] << "'" << endl);
          VERBOSE(3,"XML TAG PROB IS: '" << altProbs[0] << "'" << endl);
          VERBOSE(3,"XML TAG SPAN IS: " << startPos << "-" << (endPos-1) << endl);
          if (altProbs.size() > 0 && altTexts.size() != altProbs.size()) {
            TRACE_ERR("ERROR: Unequal number of probabilities and translation alternatives: " << line << endl);
            return false;
          }

          // store translation options into members
          if (staticData.GetXmlInputType() != XmlIgnore) {
            // only store options if we aren't ignoring them
            for (size_t i=0; i<altTexts.size(); ++i) {
              Phrase sourcePhrase; // TODO don't know what the source phrase is

              // set default probability
              float probValue = 1;
              if (altProbs.size() > 0) probValue = Scan<float>(altProbs[i]);
              // convert from prob to log-prob
              float scoreValue = FloorScore(TransformScore(probValue));

              WordsRange range(startPos + offset,endPos-1 + offset); // span covered by phrase
              TargetPhrase targetPhrase;
              targetPhrase.CreateFromString(Output, outputFactorOrder,altTexts[i],factorDelimiter, NULL);

              // lhs
              const UnknownLHSList &lhsList = staticData.GetUnknownLHS();
              if (!lhsList.empty()) {
                const Factor *factor = FactorCollection::Instance().AddFactor(lhsList[0].first);
                Word *targetLHS = new Word(true);
                targetLHS->SetFactor(0, factor); // TODO - other factors too?
                targetPhrase.SetTargetLHS(targetLHS);
              }

              targetPhrase.SetXMLScore(scoreValue);
              targetPhrase.Evaluate(sourcePhrase);

              XmlOption *option = new XmlOption(range,targetPhrase);
              assert(option);

              res.push_back(option);
            }
            altTexts.clear();
            altProbs.clear();
          }
        }
      }
    }
  }
  // we are done. check if there are tags that are still open
  if (tagStack.size() > 0) {
    TRACE_ERR("ERROR: some opened tags were never closed: " << line << endl);
    return false;
  }

  // return de-xml'ed sentence in line
  line = cleanLine;
  return true;
}

}
