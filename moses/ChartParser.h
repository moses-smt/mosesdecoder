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

#pragma once

#include <list>
#include <vector>
#include "WordsRange.h"
#include "StackVec.h"
#include "InputPath.h"

namespace Moses
{

class ChartParserCallback;
class ChartRuleLookupManager;
class InputType;
class Sentence;
class ChartCellCollectionBase;
class Word;
class Phrase;
class TargetPhraseCollection;
class DecodeGraph;

class ChartParserUnknown
{
public:
  ChartParserUnknown();
  ~ChartParserUnknown();

  void Process(const Word &sourceWord, const WordsRange &range, ChartParserCallback &to);

  const std::vector<Phrase*> &GetUnknownSources() const {
    return m_unksrcs;
  }

private:
  std::vector<Phrase*> m_unksrcs;
  std::list<TargetPhraseCollection*> m_cacheTargetPhraseCollection;
};

class ChartParser
{
public:
  ChartParser(const InputType &source, ChartCellCollectionBase &cells);
  ~ChartParser();

  void Create(const WordsRange &range, ChartParserCallback &to);

  //! the sentence being decoded
  //const Sentence &GetSentence() const;
  long GetTranslationId() const;
  size_t GetSize() const;
  const InputPath &GetInputPath(size_t startPos, size_t endPos) const;
  const InputPath &GetInputPath(const WordsRange &range) const;
  const std::vector<Phrase*> &GetUnknownSources() const {
    return m_unknown.GetUnknownSources();
  }

private:
  ChartParserUnknown m_unknown;
  std::vector <DecodeGraph*> m_decodeGraphList;
  std::vector<ChartRuleLookupManager*> m_ruleLookupManagers;
  InputType const& m_source; /**< source sentence to be translated */

  typedef std::vector< std::vector<InputPath*> > InputPathMatrix;
  InputPathMatrix	m_inputPathMatrix;

  void CreateInputPaths(const InputType &input);
  InputPath &GetInputPath(size_t startPos, size_t endPos);

};

}

