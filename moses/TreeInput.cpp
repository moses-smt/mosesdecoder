// $Id$

#include "TreeInput.h"
#include "StaticData.h"
#include "Util.h"
#include "XmlOption.h"
#include "FactorCollection.h"

using namespace std;

namespace Moses
{

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
bool TreeInput::ProcessAndStripXMLTags(string &line, std::vector<XMLParseOutput> &sourceLabels, std::vector<XmlOption*> &xmlOptions)
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

  // keep this handy for later
  const vector<FactorType> &outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();
  const string &factorDelimiter = StaticData::Instance().GetFactorDelimiter();

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
      cleanLine += xmlTokens[xmlTokenPos]; // add to output
      wordPos = Tokenize(cleanLine).size(); // count all the words
    }

    // process xml tag
    else {
      // *** get essential information about tag ***

      // strip extra boundary spaces and "<" and ">"
      string tag =  Trim(TrimXml(xmlTokens[xmlTokenPos]));
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
        TRACE_ERR("ERROR: can't have both closed and unary tag <" << tag << ">: " << line << endl);
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

        if (startPos >= endPos) {
          TRACE_ERR("ERROR: tag " << tagName << " must span at least one word: " << line << endl);
          return false;
        }

        // may be either a input span label ("label"), or a specified output translation "translation"
        string label = ParseXmlTagAttribute(tagContent,"label");
        string translation = ParseXmlTagAttribute(tagContent,"translation");

        // specified label
        if (translation.length() == 0 && label.length() > 0) {
          WordsRange range(startPos,endPos-1); // really?
          XMLParseOutput item(label, range);
          sourceLabels.push_back(item);
        }

        // specified translations -> vector of phrases, separated by "||"
        if (translation.length() > 0 && StaticData::Instance().GetXmlInputType() != XmlIgnore) {
          vector<string> altTexts = TokenizeMultiCharSeparator(translation, "||");
          vector<string> altLabel = TokenizeMultiCharSeparator(label, "||");
          vector<string> altProbs = TokenizeMultiCharSeparator(ParseXmlTagAttribute(tagContent,"prob"), "||");
          //TRACE_ERR("number of translations: " << altTexts.size() << endl);
          for (size_t i=0; i<altTexts.size(); ++i) {
            // set target phrase
            TargetPhrase targetPhrase;
            targetPhrase.CreateFromString(Output, outputFactorOrder,altTexts[i],factorDelimiter, NULL);

            // set constituent label
            string targetLHSstr;
            if (altLabel.size() > i && altLabel[i].size() > 0) {
              targetLHSstr = altLabel[i];
            } else {
              const UnknownLHSList &lhsList = StaticData::Instance().GetUnknownLHS();
              UnknownLHSList::const_iterator iterLHS = lhsList.begin();
              targetLHSstr = iterLHS->first;
            }
            Word *targetLHS = new Word(true);
            targetLHS->CreateFromString(Output, outputFactorOrder, targetLHSstr, true);
            UTIL_THROW_IF2(targetLHS->GetFactor(0) == NULL,
            		"Null factor left-hand-side");
            targetPhrase.SetTargetLHS(targetLHS);

            // not tested
            Phrase sourcePhrase = this->GetSubString(WordsRange(startPos,endPos-1));

            // get probability
            float probValue = 1;
            if (altProbs.size() > i && altProbs[i].size() > 0) {
              probValue = Scan<float>(altProbs[i]);
            }
            // convert from prob to log-prob
            float scoreValue = FloorScore(TransformScore(probValue));
            targetPhrase.SetXMLScore(scoreValue);
            targetPhrase.Evaluate(sourcePhrase);

            // set span and create XmlOption
            WordsRange range(startPos+1,endPos);
            XmlOption *option = new XmlOption(range,targetPhrase);
            assert(option);
            xmlOptions.push_back(option);

            VERBOSE(2,"xml translation = [" << range << "] " << targetLHSstr << " -> " << altTexts[i] << " prob: " << probValue << endl);
          }
          altTexts.clear();
          altProbs.clear();
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

//! populate this InputType with data from in stream
int TreeInput::Read(std::istream& in,const std::vector<FactorType>& factorOrder)
{
  const StaticData &staticData = StaticData::Instance();

  string line;
  if (getline(in, line, '\n').eof())
    return 0;
  // remove extra spaces
  //line = Trim(line);

  std::vector<XMLParseOutput> sourceLabels;
  ProcessAndStripXMLTags(line, sourceLabels, m_xmlOptions);

  // do words 1st - hack
  stringstream strme;
  strme << line << endl;

  Sentence::Read(strme, factorOrder);

  // size input chart
  size_t sourceSize = GetSize();
  m_sourceChart.resize(sourceSize);

  for (size_t pos = 0; pos < sourceSize; ++pos) {
    m_sourceChart[pos].resize(sourceSize - pos);
  }

  // do source labels
  vector<XMLParseOutput>::const_iterator iterLabel;
  for (iterLabel = sourceLabels.begin(); iterLabel != sourceLabels.end(); ++iterLabel) {
    const XMLParseOutput &labelItem = *iterLabel;
    const WordsRange &range = labelItem.m_range;
    const string &label = labelItem.m_label;
    AddChartLabel(range.GetStartPos() + 1, range.GetEndPos() + 1, label, factorOrder);
  }

  // default label
  for (size_t startPos = 0; startPos < sourceSize; ++startPos) {
    for (size_t endPos = startPos; endPos < sourceSize; ++endPos) {
      AddChartLabel(startPos, endPos, staticData.GetInputDefaultNonTerminal(), factorOrder);
    }
  }

  return 1;
}

//! Output debugging info to stream out
void TreeInput::Print(std::ostream &out) const
{
  out << *this << "\n";
}

//! create trans options specific to this InputType
TranslationOptionCollection* TreeInput::CreateTranslationOptionCollection() const
{

  return NULL;
}

void TreeInput::AddChartLabel(size_t startPos, size_t endPos, const Word &label
                              , const std::vector<FactorType>& /* factorOrder */)
{
  UTIL_THROW_IF2(!label.IsNonTerminal(),
		  "Label must be a non-terminal");

  SourceLabelOverlap overlapType = StaticData::Instance().GetSourceLabelOverlap();
  NonTerminalSet &list = GetLabelSet(startPos, endPos);
  switch (overlapType) {
  case SourceLabelOverlapAdd:
    list.insert(label);
    break;
  case SourceLabelOverlapReplace:
    if (list.size() > 0) // replace existing label
      list.clear();
    list.insert(label);
    break;
  case SourceLabelOverlapDiscard:
    if (list.size() == 0)
      list.insert(label);
    break;
  }
}

void TreeInput::AddChartLabel(size_t startPos, size_t endPos, const string &label
                              , const std::vector<FactorType>& factorOrder)
{
  Word word(true);
  const Factor *factor = FactorCollection::Instance().AddFactor(Input, factorOrder[0], label); // TODO - no factors
  word.SetFactor(0, factor);

  AddChartLabel(startPos, endPos, word, factorOrder);
}

std::ostream& operator<<(std::ostream &out, const TreeInput &input)
{
  out<< static_cast<Phrase const&>(input) << " ||| ";

  size_t size = input.GetSize();
  for (size_t startPos = 0; startPos < size; ++startPos) {
    for (size_t endPos = startPos; endPos < size; ++endPos) {
      const NonTerminalSet &labelSet = input.GetLabelSet(startPos, endPos);
      NonTerminalSet::const_iterator iter;
      for (iter = labelSet.begin(); iter != labelSet.end(); ++iter) {
        const Word &word = *iter;
        UTIL_THROW_IF2(!word.IsNonTerminal(),
      		  "Word must be a non-terminal");
        out << "[" << startPos <<"," << endPos << "]="
            << word << "(" << word.IsNonTerminal() << ") ";
      }
    }
  }

  return out;
}


} // namespace

