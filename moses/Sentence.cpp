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

#include <stdexcept>
#include <boost/algorithm/string.hpp>

#include "Sentence.h"
#include "TranslationOptionCollectionText.h"
#include "StaticData.h"
#include "ChartTranslationOptions.h"
#include "Util.h"
#include "XmlOption.h"
#include "FactorCollection.h"

using namespace std;

namespace Moses
{

Sentence::Sentence()
  : Phrase(0)
  , InputType()
{
  const StaticData& staticData = StaticData::Instance();
  if (staticData.IsChart()) {
    m_defaultLabelSet.insert(StaticData::Instance().GetInputDefaultNonTerminal());
  }
}

Sentence::~Sentence()
{
  RemoveAllInColl(m_xmlOptions);
}

int Sentence::Read(std::istream& in,const std::vector<FactorType>& factorOrder)
{
  const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
  std::string line;
  std::map<std::string, std::string> meta;

  if (getline(in, line, '\n').eof())
    return 0;

  //get covered words - if continual-partial-translation is switched on, parse input
  const StaticData &staticData = StaticData::Instance();
  m_frontSpanCoveredLength = 0;
  m_sourceCompleted.resize(0);
  if (staticData.ContinuePartialTranslation()) {
    string initialTargetPhrase;
    string sourceCompletedStr;
    int loc1 = line.find( "|||", 0 );
    int loc2 = line.find( "|||", loc1 + 3 );
    if (loc1 > -1 && loc2 > -1) {
      initialTargetPhrase = line.substr(0, loc1);
      sourceCompletedStr = line.substr(loc1 + 3, loc2 - loc1 - 3);
      line = line.substr(loc2 + 3);
      sourceCompletedStr = Trim(sourceCompletedStr);
      initialTargetPhrase = Trim(initialTargetPhrase);
      m_initialTargetPhrase = initialTargetPhrase;
      int len = sourceCompletedStr.size();
      m_sourceCompleted.resize(len);
      int contiguous = 1;
      for (int i = 0; i < len; ++i) {
        if (sourceCompletedStr.at(i) == '1') {
          m_sourceCompleted[i] = true;
          if (contiguous)
            m_frontSpanCoveredLength ++;
        } else {
          m_sourceCompleted[i] = false;
          contiguous = 0;
        }
      }
    }
  }

  // remove extra spaces
  line = Trim(line);

  // if sentences is specified as "<seg id=1> ... </seg>", extract id
  meta = ProcessAndStripSGML(line);
  if (meta.find("id") != meta.end()) {
    this->SetTranslationId(atol(meta["id"].c_str()));
  }
  if (meta.find("docid") != meta.end()) {
    this->SetDocumentId(atol(meta["docid"].c_str()));
    this->SetUseTopicId(false);
    this->SetUseTopicIdAndProb(false);
  }
  if (meta.find("topic") != meta.end()) {
    vector<string> topic_params;
    boost::split(topic_params, meta["topic"], boost::is_any_of("\t "));
    if (topic_params.size() == 1) {
      this->SetTopicId(atol(topic_params[0].c_str()));
      this->SetUseTopicId(true);
      this->SetUseTopicIdAndProb(false);
    } else {
      this->SetTopicIdAndProb(topic_params);
      this->SetUseTopicId(false);
      this->SetUseTopicIdAndProb(true);
    }
  }
  if (meta.find("weight-setting") != meta.end()) {
    this->SetWeightSetting(meta["weight-setting"]);
    this->SetSpecifiesWeightSetting(true);
    staticData.SetWeightSetting(meta["weight-setting"]);
  } else {
    this->SetSpecifiesWeightSetting(false);
  }

  // parse XML markup in translation line
  //const StaticData &staticData = StaticData::Instance();
  std::vector< size_t > xmlWalls;
  std::vector< std::pair<size_t, std::string> > placeholders;

  if (staticData.GetXmlInputType() != XmlPassThrough) {
    int offset = 0;
    if (staticData.IsChart()) {
      offset = 1;
    }

    if (!ProcessAndStripXMLTags(line, m_xmlOptions, m_reorderingConstraint, xmlWalls, placeholders,
                                offset,
                                staticData.GetXmlBrackets().first,
                                staticData.GetXmlBrackets().second)) {
      const string msg("Unable to parse XML in line: " + line);
      TRACE_ERR(msg << endl);
      throw runtime_error(msg);
    }
  }

  Phrase::CreateFromString(Input, factorOrder, line, factorDelimiter, NULL);

  // placeholders
  ProcessPlaceholders(placeholders);

  if (staticData.IsChart()) {
    InitStartEndWord();
  }

  //now that we have final word positions in phrase (from CreateFromString),
  //we can make input phrase objects to go with our XmlOptions and create TranslationOptions

  //only fill the vector if we are parsing XML
  if (staticData.GetXmlInputType() != XmlPassThrough ) {
    for (size_t i=0; i<GetSize(); i++) {
      m_xmlCoverageMap.push_back(false);
    }

    //iterXMLOpts will be empty for XmlIgnore
    //look at each column
    for(std::vector<XmlOption*>::const_iterator iterXmlOpts = m_xmlOptions.begin();
        iterXmlOpts != m_xmlOptions.end(); iterXmlOpts++) {

      const XmlOption *xmlOption = *iterXmlOpts;
      const WordsRange &range = xmlOption->range;

      for(size_t j=range.GetStartPos(); j<=range.GetEndPos(); j++) {
        m_xmlCoverageMap[j]=true;
      }
    }

  }

  // reordering walls and zones
  m_reorderingConstraint.InitializeWalls( GetSize() );

  // set reordering walls, if "-monotone-at-punction" is set
  if (staticData.UseReorderingConstraint() && GetSize()>0) {
    m_reorderingConstraint.SetMonotoneAtPunctuation( GetSubString( WordsRange(0,GetSize()-1 ) ) );
  }

  // set walls obtained from xml
  for(size_t i=0; i<xmlWalls.size(); i++)
    if( xmlWalls[i] < GetSize() ) // no buggy walls, please
      m_reorderingConstraint.SetWall( xmlWalls[i], true );
  m_reorderingConstraint.FinalizeWalls();

  return 1;
}

void Sentence::ProcessPlaceholders(const std::vector< std::pair<size_t, std::string> > &placeholders)
{
  FactorType placeholderFactor = StaticData::Instance().GetPlaceholderFactor();
  if (placeholderFactor == NOT_FOUND) {
    return;
  }

  for (size_t i = 0; i < placeholders.size(); ++i) {
    size_t pos = placeholders[i].first;
    const string &str = placeholders[i].second;
    const Factor *factor = FactorCollection::Instance().AddFactor(str);
    Word &word = Phrase::GetWord(pos);
    word[placeholderFactor] = factor;
  }
}

TranslationOptionCollection*
Sentence::CreateTranslationOptionCollection() const
{
  size_t maxNoTransOptPerCoverage = StaticData::Instance().GetMaxNoTransOptPerCoverage();
  float transOptThreshold = StaticData::Instance().GetTranslationOptionThreshold();
  TranslationOptionCollection *rv= new TranslationOptionCollectionText(*this, maxNoTransOptPerCoverage, transOptThreshold);
  assert(rv);
  return rv;
}
void Sentence::Print(std::ostream& out) const
{
  out<<*static_cast<Phrase const*>(this);
}


bool Sentence::XmlOverlap(size_t startPos, size_t endPos) const
{
  for (size_t pos = startPos; pos <=  endPos ; pos++) {
    if (pos < m_xmlCoverageMap.size() && m_xmlCoverageMap[pos]) {
      return true;
    }
  }
  return false;
}

void Sentence::GetXmlTranslationOptions(std::vector <TranslationOption*> &list) const
{
  for (std::vector<XmlOption*>::const_iterator iterXMLOpts = m_xmlOptions.begin();
       iterXMLOpts != m_xmlOptions.end(); ++iterXMLOpts) {
    const XmlOption &xmlOption = **iterXMLOpts;
    const WordsRange &range = xmlOption.range;
    const TargetPhrase &targetPhrase = xmlOption.targetPhrase;
    TranslationOption *transOpt = new TranslationOption(range, targetPhrase);
    list.push_back(transOpt);
  }
}

void Sentence::GetXmlTranslationOptions(std::vector <TranslationOption*> &list, size_t startPos, size_t endPos) const
{
  //iterate over XmlOptions list, find exact source/target matches

  for (std::vector<XmlOption*>::const_iterator iterXMLOpts = m_xmlOptions.begin();
       iterXMLOpts != m_xmlOptions.end(); ++iterXMLOpts) {
    const XmlOption &xmlOption = **iterXMLOpts;
    const WordsRange &range = xmlOption.range;

    if (startPos == range.GetStartPos()
        && endPos == range.GetEndPos()) {
      const TargetPhrase &targetPhrase = xmlOption.targetPhrase;

      TranslationOption *transOpt = new TranslationOption(range, targetPhrase);
      list.push_back(transOpt);
    }
  }
}

std::vector <ChartTranslationOptions*> Sentence::GetXmlChartTranslationOptions() const
{
  const StaticData &staticData = StaticData::Instance();
  std::vector <ChartTranslationOptions*> ret;

  // XML Options
  // this code is a copy of the 1 in Sentence.

  //only fill the vector if we are parsing XML
  if (staticData.GetXmlInputType() != XmlPassThrough ) {
    //TODO: needed to handle exclusive
    //for (size_t i=0; i<GetSize(); i++) {
    //  m_xmlCoverageMap.push_back(false);
    //}

    //iterXMLOpts will be empty for XmlIgnore
    //look at each column
    for(std::vector<XmlOption*>::const_iterator iterXmlOpts = m_xmlOptions.begin();
        iterXmlOpts != m_xmlOptions.end(); iterXmlOpts++) {

      const XmlOption &xmlOption = **iterXmlOpts;
      TargetPhrase *targetPhrase = new TargetPhrase(xmlOption.targetPhrase);

      WordsRange *range = new WordsRange(xmlOption.range);
      const StackVec emptyStackVec; // hmmm... maybe dangerous, but it is never consulted

      TargetPhraseCollection *tpc = new TargetPhraseCollection;
      tpc->Add(targetPhrase);

      ChartTranslationOptions *transOpt = new ChartTranslationOptions(*tpc, emptyStackVec, *range, 0.0f);
      ret.push_back(transOpt);

      //TODO: needed to handle exclusive
      //for(size_t j=transOpt->GetSourceWordsRange().GetStartPos(); j<=transOpt->GetSourceWordsRange().GetEndPos(); j++) {
      //  m_xmlCoverageMap[j]=true;
      //}
    }
  }

  return ret;
}

void Sentence::CreateFromString(const std::vector<FactorType> &factorOrder
                                , const std::string &phraseString
                                , const std::string &factorDelimiter)
{
  Phrase::CreateFromString(Input, factorOrder, phraseString, factorDelimiter, NULL);
}



}

