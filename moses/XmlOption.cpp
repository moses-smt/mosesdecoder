// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
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
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include "Util.h"
#include "StaticData.h"
#include "Range.h"
#include "TargetPhrase.h"
#include "ReorderingConstraint.h"
#include "FactorCollection.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#if PT_UG
#include "TranslationModel/UG/mmsapt.h"
#endif

namespace Moses
{
using namespace std;
using namespace boost::algorithm;

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
  if (starts_with(str, lbrackStr) && ends_with(str, rbrackStr)) {
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
bool
ProcessAndStripXMLTags(AllOptions const& opts, string &line,
                       vector<XmlOption const*> &res,
                       ReorderingConstraint &reorderingConstraint,
                       vector< size_t > &walls,
                       std::vector< std::pair<size_t, std::string> > &placeholders,
                       InputType &input)
{
  //parse XML markup in translation line

  const std::string& lbrackStr = opts.input.xml_brackets.first;
  const std::string& rbrackStr = opts.input.xml_brackets.second;
  int offset = is_syntax(opts.search.algo) ? 1 : 0;

  // const StaticData &staticData = StaticData::Instance();

  // hack. What pt should XML trans opt be assigned to?
  PhraseDictionary *firstPt = NULL;
  if (PhraseDictionary::GetColl().size() == 0) {
    firstPt = PhraseDictionary::GetColl()[0];
  }

  // no xml tag? we're done.
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

  const vector<FactorType> &outputFactorOrder = opts.output.factor_order;

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

        // update: add new aligned sentence pair to Mmsapt identified by name
        else if (tagName == "update") {
#if PT_UG
          // get model name and aligned sentence pair
          string pdName = ParseXmlTagAttribute(tagContent,"name");
          string source = ParseXmlTagAttribute(tagContent,"source");
          string target = ParseXmlTagAttribute(tagContent,"target");
          string alignment = ParseXmlTagAttribute(tagContent,"alignment");
          // find PhraseDictionary by name
          const vector<PhraseDictionary*> &pds = PhraseDictionary::GetColl();
          PhraseDictionary* pd = NULL;
          for (vector<PhraseDictionary*>::const_iterator i = pds.begin(); i != pds.end(); ++i) {
            PhraseDictionary* curPd = *i;
            if (curPd->GetScoreProducerDescription() == pdName) {
              pd = curPd;
              break;
            }
          }
          if (pd == NULL) {
            TRACE_ERR("ERROR: No PhraseDictionary with name " << pdName << ", no update" << endl);
            return false;
          }
          // update model
          VERBOSE(3,"Updating " << pdName << " ||| " << source << " ||| " << target << " ||| " << alignment << endl);
          Mmsapt* pdsa = reinterpret_cast<Mmsapt*>(pd);
          pdsa->add(source, target, alignment);
#else
          TRACE_ERR("ERROR: recompile with --with-mm to update PhraseDictionary at runtime" << endl);
          return false;
#endif
        }

        // weight-overwrite: update feature weights, unspecified weights remain unchanged
        // IMPORTANT: translation models that cache phrases or apply table-limit during load
        // based on initial weights need to be reset.  Sending an empty update will do this
        // for PhraseDictionaryBitextSampling (Mmsapt) models:
        // <update name="TranslationModelName" source=" " target=" " alignment=" " />
        else if (tagName == "weight-overwrite") {

          // is a name->ff map stored anywhere so we don't have to build it every time?
          const vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
          boost::unordered_map<string, FeatureFunction*> map;
          BOOST_FOREACH(FeatureFunction* const& ff, ffs) {
            map[ff->GetScoreProducerDescription()] = ff;
          }

          // update each weight listed
          ScoreComponentCollection allWeights = StaticData::Instance().GetAllWeights();
          boost::unordered_map<string, FeatureFunction*>::iterator ffi;
          string ffName("");
          vector<float> ffWeights;
          vector<string> toks = Tokenize(ParseXmlTagAttribute(tagContent,"weights"));
          BOOST_FOREACH(string const& tok, toks) {
            if (ends_with(tok, "=")) {
              // start new feature
              if (ffName != "") {
                // set previous feature weights
                if (ffi != map.end()) {
                  allWeights.Assign(ffi->second, ffWeights);
                }
                ffWeights.clear();
              }
              ffName = tok.substr(0, tok.size() - 1);
              ffi = map.find(ffName);
              if (ffi == map.end()) {
                TRACE_ERR("ERROR: No FeatureFunction with name " << ffName << ", no weight update" << endl);
              }
            } else {
              // weight for current feature
              ffWeights.push_back(Scan<float>(tok));
            }
          }
          if (ffi != map.end()) {
            allWeights.Assign(ffi->second, ffWeights);
          }
          StaticData::InstanceNonConst().SetAllWeights(allWeights);
        }

        // Coord: coordinates of the input sentence in a user-defined space
        // <coord space="NAME" coord="X Y Z ..." />
        // where NAME is the name of the space and X Y Z ... are floats.  See
        // PhraseDistanceFeature for an example of using this information for
        // feature scoring.
        else if (tagName == "coord") {
          // Parse tag
          string space = ParseXmlTagAttribute(tagContent, "space");
          vector<string> tok = Tokenize(ParseXmlTagAttribute(tagContent, "coord"));
          size_t id = StaticData::Instance().GetCoordSpace(space);
          if (!id) {
            TRACE_ERR("ERROR: no models use space " << space << ", will be ignored" << endl);
          } else {
            // Init if needed
            if (!input.m_coordMap) {
              input.m_coordMap.reset(new map<size_t const, vector<float> >);
            }
            vector<float>& coord = (*input.m_coordMap)[id];
            Scan<float>(coord, tok);
          }
        }

        // default: opening tag that specifies translation options
        else {
          if (startPos > endPos) {
            TRACE_ERR("ERROR: tag " << tagName << " startPos > endPos: " << line << endl);
            return false;
          } else if (startPos == endPos) {
            TRACE_ERR("WARNING: tag " << tagName << " 0 span: " << line << endl);
            continue;
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
          if (opts.input.xml_policy != XmlIgnore) {
            // only store options if we aren't ignoring them
            for (size_t i=0; i<altTexts.size(); ++i) {
              Phrase sourcePhrase; // TODO don't know what the source phrase is

              // set default probability
              float probValue = 1;
              if (altProbs.size() > 0) probValue = Scan<float>(altProbs[i]);
              // convert from prob to log-prob
              float scoreValue = FloorScore(TransformScore(probValue));

              Range range(startPos + offset,endPos-1 + offset); // span covered by phrase
              TargetPhrase targetPhrase(firstPt);
              // Target factors may be used by intermediate models (example: a
              // generation model produces a factor used by a class-based LM
              // but NOT output.  Fake the output factor order to match the
              // number of factors specified in the alt text.  A one-factor
              // system would have "word", a two-factor system would have
              // "word|class", and so on.
              vector<FactorType> fakeOutputFactorOrder;
              // Factors in first word of alt text
              size_t factorsInAltText = TokenizeMultiCharSeparator(Tokenize(altTexts[i])[0], StaticData::Instance().GetFactorDelimiter()).size();
              for (size_t f = 0; f < factorsInAltText; ++f) {
                fakeOutputFactorOrder.push_back(f);
              }
              targetPhrase.CreateFromString(Output, fakeOutputFactorOrder, altTexts[i], NULL);

              // lhs
              const UnknownLHSList &lhsList = opts.syntax.unknown_lhs; // staticData.GetUnknownLHS();
              if (!lhsList.empty()) {
                const Factor *factor = FactorCollection::Instance().AddFactor(lhsList[0].first, true);
                Word *targetLHS = new Word(true);
                targetLHS->SetFactor(0, factor); // TODO - other factors too?
                targetPhrase.SetTargetLHS(targetLHS);
              }

              targetPhrase.SetXMLScore(scoreValue);
              targetPhrase.EvaluateInIsolation(sourcePhrase);

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
