// $Id: XmlOption.cpp 1960 2008-12-15 12:52:38Z phkoehn $
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

#include <cassert>
#include <vector>
#include <string>
#include <set>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include "SyntaxTree.h"
#include "XmlException.h"

using namespace std;

namespace MosesTraining
{

inline std::vector<std::string> Tokenize(const std::string& str,
    const std::string& delimiters = " \t")
{
  std::vector<std::string> tokens;
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }

  return tokens;
}

std::string Trim(const std::string& str, const std::string dropChars = " \t\n\r")
{
  std::string res = str;
  res.erase(str.find_last_not_of(dropChars)+1);
  return res.erase(0, res.find_first_not_of(dropChars));
}

string ParseXmlTagAttribute(const string& tag,const string& attributeName)
{
  /*TODO deal with unescaping \"*/
  string tagOpen = attributeName + "=\"";
  size_t contentsStart = tag.find(tagOpen);
  if (contentsStart == string::npos) return "";
  contentsStart += tagOpen.size();
  size_t contentsEnd = tag.find_first_of('"',contentsStart+1);
  if (contentsEnd == string::npos) {
    cerr << "Malformed XML attribute: "<< tag;
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
 */
string TrimXml(const string& str)
{
  // too short to be xml token -> do nothing
  if (str.size() < 2) return str;

  // strip first and last character
  if (str[0] == '<' && str[str.size() - 1] == '>') {
    return str.substr(1, str.size() - 2);
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
 */
bool isXmlTag(const string& tag)
{
  return tag[0] == '<';
}

/**
 * Unescape XML special characters.
 */
string unescape(const string& str)
{
  string s;
  s.reserve(str.size());
  string::size_type n;
  string::size_type start = 0;
  while ((n = str.find('&', start)) != string::npos) {
    s += str.substr(start, n-start);
    string::size_type end = str.find(';', n);
    assert(n != string::npos);
    string name = str.substr(n+1, end-n-1);
    if (name == "lt") {
      s += string("<");
    } else if (name == "gt") {
      s += string(">");
    } else if (name == "#91") {
      s += string("[");
    } else if (name == "#93") {
      s += string("]");
    } else if (name == "bra") {
      s += string("[");
    } else if (name == "ket") {
      s += string("]");
    } else if (name == "bar") {
      s += string("|");
    } else if (name == "amp") {
      s += string("&");
    } else if (name == "apos") {
      s += string("'");
    } else if (name == "quot") {
      s += string("\"");
    } else {
      // Currently only handles the following five XML escape sequences:
      //      &lt;        <
      //      &gt;        >
      //      &amp;       &
      //      &apos;      '
      //      &quot;      "
      // Numeric character references (like &#xf6;) are not supported.
      std::ostringstream msg;
      msg << "unsupported XML escape sequence: &" << name << ";";
      throw XmlException(msg.str());
    }
    if (end == str.size()-1) {
      return s;
    }
    start = end + 1;
  }
  s += str.substr(start);
  return s;
}

/**
 * Split up the input character string into tokens made up of
 * either XML tags or text.
 * example: this <b> is a </b> test .
 *       => (this ), (<b>), ( is a ), (</b>), ( test .)
 *
 * \param str input string
 */
vector<string> TokenizeXml(const string& str)
{
  string lbrack = "<";
  string rbrack = ">";
  vector<string> tokens; // vector of tokens to be returned
  string::size_type cpos = 0; // current position in string
  string::size_type lpos = 0; // left start of xml tag
  string::size_type rpos = 0; // right end of xml tag

  // walk thorugh the string (loop vver cpos)
  while (cpos != str.size()) {
    // find the next opening "<" of an xml tag
    lpos = str.find_first_of(lbrack, cpos);
    if (lpos != string::npos) {
      // find the end of the xml tag
      rpos = str.find_first_of(rbrack, lpos);
      // sanity check: there has to be closing ">"
      if (rpos == string::npos) {
        cerr << "ERROR: malformed XML: " << str << endl;
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
    tokens.push_back(str.substr(lpos, rpos-lpos+1));
    cpos = rpos + 1;
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
 */
/*TODO: we'd only have to return a vector of XML options if we dropped linking. 2-d vector
	is so we can link things up afterwards. We can't create TranslationOptions as we
	parse because we don't have the completed source parsed until after this function
	removes all the markup from it (CreateFromString in Sentence::Read).
*/
bool ProcessAndStripXMLTags(string &line, SyntaxTree &tree, set< string > &labelCollection, map< string, int > &topLabelCollection, bool unescapeSpecialChars )
{
  //parse XML markup in translation line

  // no xml tag? we're done.
  if (line.find_first_of('<') == string::npos) {
    return true;
  }

  // break up input into a vector of xml tags and text
  // example: (this), (<b>), (is a), (</b>), (test .)
  vector<string> xmlTokens = TokenizeXml(line);

  // we need to store opened tags, until they are closed
  // tags are stored as tripled (tagname, startpos, contents)
  typedef pair< string, pair< size_t, string > > OpenedTag;
  vector< OpenedTag > tagStack; // stack that contains active opened tags

  string cleanLine; // return string (text without xml)
  size_t wordPos = 0; // position in sentence (in terms of number of words)
  bool isLinked = false;

  // loop through the tokens
  for (size_t xmlTokenPos = 0 ; xmlTokenPos < xmlTokens.size() ; xmlTokenPos++) {
    // not a xml tag, but regular text (may contain many words)
    if(!isXmlTag(xmlTokens[xmlTokenPos])) {
      // add a space at boundary, if necessary
      if (cleanLine.size()>0 &&
          cleanLine[cleanLine.size() - 1] != ' ' &&
          xmlTokens[xmlTokenPos][0] != ' ') {
        cleanLine += " ";
      }
      // add words to output
      if (unescapeSpecialChars) {
        cleanLine += unescape(xmlTokens[xmlTokenPos]);
      } else {
        cleanLine += xmlTokens[xmlTokenPos];
      }
      wordPos = Tokenize(cleanLine).size(); // count all the words
    }

    // process xml tag
    else {
      // *** get essential information about tag ***

      // strip extra boundary spaces and "<" and ">"
      string tag =  Trim(TrimXml(xmlTokens[xmlTokenPos]));
      // cerr << "XML TAG IS: " << tag << std::endl;

      if (tag.size() == 0) {
        cerr << "ERROR: empty tag name: " << line << endl;
        return false;
      }

      // check if unary (e.g., "<wall/>")
      bool isUnary = ( tag[tag.size() - 1] == '/' );

      // check if opening tag (e.g. "<a>", not "</a>")g
      bool isClosed = ( tag[0] == '/' );
      bool isOpen = !isClosed;

      if (isClosed && isUnary) {
        cerr << "ERROR: can't have both closed and unary tag <" << tag << ">: " << line << endl;
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
        // cerr << "XML TAG " << tagName << " (" << tagContent << ") added to stack, now size " << tagStack.size() << endl;
      }

      // *** process completed tag ***

      if (isClosed || isUnary) {
        // pop last opened tag from stack;
        if (tagStack.size() == 0) {
          cerr << "ERROR: tag " << tagName << " closed, but not opened" << ":" << line << endl;
          return false;
        }
        OpenedTag openedTag = tagStack.back();
        tagStack.pop_back();

        // tag names have to match
        if (openedTag.first != tagName) {
          cerr << "ERROR: tag " << openedTag.first << " closed by tag " << tagName << ": " << line << endl;
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
            cerr << "ERROR: span attribute must be of the form \"i-j\" or \"i\": " << line << endl;
            return false;
          }
          startPos = atoi(ij[0].c_str());
          if (ij.size() == 1) endPos = startPos + 1;
          else endPos = atoi(ij[1].c_str()) + 1;
        }

        // cerr << "XML TAG " << tagName << " (" << tagContent << ") spanning " << startPos << " to " << (endPos-1) << " complete, commence processing" << endl;

        if (startPos >= endPos) {
          cerr << "ERROR: tag " << tagName << " must span at least one word (" << startPos << "-" << endPos << "): " << line << endl;
          return false;
        }

        string label = ParseXmlTagAttribute(tagContent,"label");
        labelCollection.insert( label );

        string pcfgString = ParseXmlTagAttribute(tagContent,"pcfg");
        float pcfgScore = pcfgString == "" ? 0.0f
                          : std::atof(pcfgString.c_str());

        // report what we have processed so far
        if (0) {
          cerr << "XML TAG NAME IS: '" << tagName << "'" << endl;
          cerr << "XML TAG LABEL IS: '" << label << "'" << endl;
          cerr << "XML SPAN IS: " << startPos << "-" << (endPos-1) << endl;
        }
        SyntaxNode *node = tree.AddNode( startPos, endPos-1, label );
        node->SetPcfgScore(pcfgScore);
      }
    }
  }
  // we are done. check if there are tags that are still open
  if (tagStack.size() > 0) {
    cerr << "ERROR: some opened tags were never closed: " << line << endl;
    return false;
  }

  // collect top labels
  const vector< SyntaxNode* >& topNodes = tree.GetNodes( 0, wordPos-1 );
  for( vector< SyntaxNode* >::const_iterator node = topNodes.begin(); node != topNodes.end(); node++ ) {
    SyntaxNode *n = *node;
    const string &label = n->GetLabel();
    if (topLabelCollection.find( label ) == topLabelCollection.end())
      topLabelCollection[ label ] = 0;
    topLabelCollection[ label ]++;
  }

  // return de-xml'ed sentence in line
  line = cleanLine;
  return true;
}

}
