// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include "ChartParser.h"
#include "ChartParserCallback.h"
#include "ChartRuleLookupManager.h"
#include "StaticData.h"
#include "TreeInput.h"
#include "Sentence.h"
#include "DecodeGraph.h"
#include "moses/TranslationModel/PhraseDictionary.h"

using namespace std;
using namespace Moses;

namespace Moses
{

ChartParser::ChartParser(InputType const &source, ChartCellCollectionBase &cells) :
  m_decodeGraphList(StaticData::Instance().GetDecodeGraphs()),
  m_source(source)
{
  const StaticData &staticData = StaticData::Instance();

  staticData.InitializeForInput(source);
  CreateInputPaths(m_source);

  const std::vector<PhraseDictionary*> &dictionaries = PhraseDictionary::GetColl();
  assert(dictionaries.size() == m_decodeGraphList.size());
  m_ruleLookupManagers.reserve(dictionaries.size());
  for (std::size_t i = 0; i < dictionaries.size(); ++i) {
    const PhraseDictionary *dict = dictionaries[i];
    PhraseDictionary *nonConstDict = const_cast<PhraseDictionary*>(dict);
    std::size_t maxChartSpan = m_decodeGraphList[i]->GetMaxChartSpan();
    ChartRuleLookupManager *lookupMgr = nonConstDict->CreateRuleLookupManager(*this, cells, maxChartSpan);
    m_ruleLookupManagers.push_back(lookupMgr);
  }

}

ChartParser::~ChartParser()
{
  RemoveAllInColl(m_ruleLookupManagers);
  StaticData::Instance().CleanUpAfterSentenceProcessing(m_source);

  InputPathMatrix::const_iterator iterOuter;
  for (iterOuter = m_inputPathMatrix.begin(); iterOuter != m_inputPathMatrix.end(); ++iterOuter) {
    const std::vector<InputPath*> &outer = *iterOuter;

    std::vector<InputPath*>::const_iterator iterInner;
    for (iterInner = outer.begin(); iterInner != outer.end(); ++iterInner) {
      InputPath *path = *iterInner;
      delete path;
    }
  }
}

void ChartParser::Create(const WordsRange &wordsRange, ChartParserCallback &to)
{
  assert(m_decodeGraphList.size() == m_ruleLookupManagers.size());

  std::vector <DecodeGraph*>::const_iterator iterDecodeGraph;
  std::vector <ChartRuleLookupManager*>::const_iterator iterRuleLookupManagers = m_ruleLookupManagers.begin();
  for (iterDecodeGraph = m_decodeGraphList.begin(); iterDecodeGraph != m_decodeGraphList.end(); ++iterDecodeGraph, ++iterRuleLookupManagers) {
    const DecodeGraph &decodeGraph = **iterDecodeGraph;
    assert(decodeGraph.GetSize() == 1);
    ChartRuleLookupManager &ruleLookupManager = **iterRuleLookupManagers;
    size_t maxSpan = decodeGraph.GetMaxChartSpan();
    size_t last = m_source.GetSize()-1;
    if (maxSpan != 0) {
      last = min(last, wordsRange.GetStartPos()+maxSpan);
    }
    if (maxSpan == 0 || wordsRange.GetNumWordsCovered() <= maxSpan) {
      const InputPath &inputPath = GetInputPath(wordsRange);
      ruleLookupManager.GetChartRuleCollection(inputPath, last, to);
    }
  }

}

void ChartParser::CreateInputPaths(const InputType &input)
{
  size_t size = input.GetSize();
  m_inputPathMatrix.resize(size);

  UTIL_THROW_IF2(input.GetType() != SentenceInput && input.GetType() != TreeInputType,
                 "Input must be a sentence or a tree, not lattice or confusion networks");
  for (size_t phaseSize = 1; phaseSize <= size; ++phaseSize) {
    for (size_t startPos = 0; startPos < size - phaseSize + 1; ++startPos) {
      size_t endPos = startPos + phaseSize -1;
      vector<InputPath*> &vec = m_inputPathMatrix[startPos];

      WordsRange range(startPos, endPos);
      Phrase subphrase(input.GetSubString(WordsRange(startPos, endPos)));
      const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

      InputPath *node;
      if (range.GetNumWordsCovered() == 1) {
        node = new InputPath(subphrase, labels, range, NULL, NULL);
        vec.push_back(node);
      } else {
        const InputPath &prevNode = GetInputPath(startPos, endPos - 1);
        node = new InputPath(subphrase, labels, range, &prevNode, NULL);
        vec.push_back(node);
      }

      //m_inputPathQueue.push_back(node);
    }
  }
}

const InputPath &ChartParser::GetInputPath(const WordsRange &range) const
{
  return GetInputPath(range.GetStartPos(), range.GetEndPos());
}

const InputPath &ChartParser::GetInputPath(size_t startPos, size_t endPos) const
{
  size_t offset = endPos - startPos;
  UTIL_THROW_IF2(offset >= m_inputPathMatrix[startPos].size(),
                 "Out of bound: " << offset);
  return *m_inputPathMatrix[startPos][offset];
}

InputPath &ChartParser::GetInputPath(size_t startPos, size_t endPos)
{
  size_t offset = endPos - startPos;
  UTIL_THROW_IF2(offset >= m_inputPathMatrix[startPos].size(),
                 "Out of bound: " << offset);
  return *m_inputPathMatrix[startPos][offset];
}
/*
const Sentence &ChartParser::GetSentence() const {
  const Sentence &sentence = static_cast<const Sentence&>(m_source);
  return sentence;
}
*/
size_t ChartParser::GetSize() const
{
  return m_source.GetSize();
}

long ChartParser::GetTranslationId() const
{
  return m_source.GetTranslationId();
}
} // namespace Moses
