/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#include "LoaderCompact.h"

#include "moses/AlignmentInfoCollection.h"
#include "moses/InputFileStream.h"
#include "moses/UserMessage.h"
#include "moses/Util.h"
#include "moses/Word.h"
#include "Trie.h"

#include <istream>
#include <sstream>

namespace Moses
{

bool RuleTableLoaderCompact::Load(const std::vector<FactorType> &input,
                                  const std::vector<FactorType> &output,
                                  const std::string &inFile,
                                  size_t /* tableLimit */,
                                  RuleTableTrie &ruleTable)
{
  PrintUserTime("Start loading compact rule table");

  InputFileStream inStream(inFile);
  LineReader reader(inStream);

  // Read and check version number.
  reader.ReadLine();
  if (reader.m_line != "1") {
    std::stringstream msg;
    msg << "Unexpected compact rule table format: " << reader.m_line;
    UserMessage::Add(msg.str());
    return false;
  }

  // Load vocabulary.
  std::vector<Word> vocab;
  LoadVocabularySection(reader, input, vocab);

  // Load source phrases.
  std::vector<Phrase> sourcePhrases;
  std::vector<size_t> sourceLhsIds;
  LoadPhraseSection(reader, vocab, sourcePhrases, sourceLhsIds);

  // Load target phrases.
  std::vector<Phrase> targetPhrases;
  std::vector<size_t> targetLhsIds;
  LoadPhraseSection(reader, vocab, targetPhrases, targetLhsIds);

  // Load alignments.
  std::vector<const AlignmentInfo *> alignmentSets;
  LoadAlignmentSection(reader, alignmentSets, sourcePhrases);

  // Load rules.
  if (!LoadRuleSection(reader, vocab, sourcePhrases, targetPhrases,
                       targetLhsIds, alignmentSets,
                       ruleTable)) {
    return false;
  }

  // Sort and prune each target phrase collection.
  SortAndPrune(ruleTable);

  return true;
}

void RuleTableLoaderCompact::LoadVocabularySection(
  LineReader &reader,
  const std::vector<FactorType> &factorTypes,
  std::vector<Word> &vocabulary)
{
  // Read symbol count.
  reader.ReadLine();
  const size_t vocabSize = std::atoi(reader.m_line.c_str());

  // Read symbol lines and create Word objects.
  vocabulary.resize(vocabSize);
  for (size_t i = 0; i < vocabSize; ++i) {
    reader.ReadLine();
    const size_t len = reader.m_line.size();
    bool isNonTerm = (reader.m_line[0] == '[' && reader.m_line[len-1] == ']');
    if (isNonTerm) {
      reader.m_line = reader.m_line.substr(1, len-2);
    }
    vocabulary[i].CreateFromString(Input, factorTypes, reader.m_line, isNonTerm);
  }
}

void RuleTableLoaderCompact::LoadPhraseSection(
  LineReader &reader,
  const std::vector<Word> &vocab,
  std::vector<Phrase> &rhsPhrases,
  std::vector<size_t> &lhsIds)
{
  // Read phrase count.
  reader.ReadLine();
  const size_t phraseCount = std::atoi(reader.m_line.c_str());

  // Reads lines, storing Phrase object for each RHS and vocab ID for each LHS.
  rhsPhrases.resize(phraseCount, Phrase(0));
  lhsIds.resize(phraseCount);
  std::vector<size_t> tokenPositions;
  for (size_t i = 0; i < phraseCount; ++i) {
    reader.ReadLine();
    tokenPositions.clear();
    FindTokens(tokenPositions, reader.m_line);
    const char *charLine = reader.m_line.c_str();
    lhsIds[i] = std::atoi(charLine+tokenPositions[0]);
    for (size_t j = 1; j < tokenPositions.size(); ++j) {
      rhsPhrases[i].AddWord(vocab[std::atoi(charLine+tokenPositions[j])]);
    }
  }
}

void RuleTableLoaderCompact::LoadAlignmentSection(
  LineReader &reader, std::vector<const AlignmentInfo *> &alignmentSets, std::vector<Phrase> &sourcePhrases)
{
  // Read alignment set count.
  reader.ReadLine();
  const size_t alignmentSetCount = std::atoi(reader.m_line.c_str());

  alignmentSets.resize(alignmentSetCount * 2);
  AlignmentInfo::CollType alignTerm, alignNonTerm;
  std::vector<std::string> tokens;
  std::vector<size_t> points;
  for (size_t i = 0; i < alignmentSetCount; ++i) {
    // Read alignment set, lookup in collection, and store pointer.
    alignTerm.clear();
    alignNonTerm.clear();
    tokens.clear();

    reader.ReadLine();
    Tokenize(tokens, reader.m_line);
    std::vector<std::string>::const_iterator p;
    for (p = tokens.begin(); p != tokens.end(); ++p) {
      points.clear();
      Tokenize<size_t>(points, *p, "-");
      std::pair<size_t, size_t> alignmentPair(points[0], points[1]);

      if (sourcePhrases[i].GetWord(alignmentPair.first).IsNonTerminal()) {
        alignNonTerm.insert(alignmentPair);
      } else {
        alignTerm.insert(alignmentPair);
      }

    }
    alignmentSets[i*2] = AlignmentInfoCollection::Instance().Add(alignNonTerm);
    alignmentSets[i*2 + 1] = AlignmentInfoCollection::Instance().Add(alignTerm);
  }
}

bool RuleTableLoaderCompact::LoadRuleSection(
  LineReader &reader,
  const std::vector<Word> &vocab,
  const std::vector<Phrase> &sourcePhrases,
  const std::vector<Phrase> &targetPhrases,
  const std::vector<size_t> &targetLhsIds,
  const std::vector<const AlignmentInfo *> &alignmentSets,
  RuleTableTrie &ruleTable)
{
  // Read rule count.
  reader.ReadLine();
  const size_t ruleCount = std::atoi(reader.m_line.c_str());

  // Read rules and add to table.
  const size_t numScoreComponents = ruleTable.GetNumScoreComponents();
  std::vector<float> scoreVector(numScoreComponents);
  std::vector<size_t> tokenPositions;
  for (size_t i = 0; i < ruleCount; ++i) {
    reader.ReadLine();

    tokenPositions.clear();
    FindTokens(tokenPositions, reader.m_line);

    const char *charLine = reader.m_line.c_str();

    // The first three tokens are IDs for the source phrase, target phrase,
    // and alignment set.
    const int sourcePhraseId = std::atoi(charLine+tokenPositions[0]);
    const int targetPhraseId = std::atoi(charLine+tokenPositions[1]);
    const int alignmentSetId = std::atoi(charLine+tokenPositions[2]);

    const Phrase &sourcePhrase = sourcePhrases[sourcePhraseId];
    const Phrase &targetPhrasePhrase = targetPhrases[targetPhraseId];
    const Word *targetLhs = new Word(vocab[targetLhsIds[targetPhraseId]]);
    Word sourceLHS("X"); // TODO not implemented for compact
    const AlignmentInfo *alignNonTerm = alignmentSets[alignmentSetId];

    // Then there should be one score for each score component.
    for (size_t j = 0; j < numScoreComponents; ++j) {
      float score = std::atof(charLine+tokenPositions[3+j]);
      scoreVector[j] = FloorScore(TransformScore(score));
    }
    if (reader.m_line[tokenPositions[3+numScoreComponents]] != ':') {
      std::stringstream msg;
      msg << "Size of scoreVector != number ("
          << scoreVector.size() << "!=" << numScoreComponents
          << ") of score components on line " << reader.m_lineNum;
      UserMessage::Add(msg.str());
      return false;
    }

    // The remaining columns are currently ignored.

    // Create and score target phrase.
    TargetPhrase *targetPhrase = new TargetPhrase(targetPhrasePhrase);
    targetPhrase->SetAlignNonTerm(alignNonTerm);
    targetPhrase->SetTargetLHS(targetLhs);

    targetPhrase->Evaluate(sourcePhrase, ruleTable.GetFeaturesToApply());

    // Insert rule into table.
    TargetPhraseCollection &coll = GetOrCreateTargetPhraseCollection(
                                     ruleTable, sourcePhrase, *targetPhrase, &sourceLHS);
    coll.Add(targetPhrase);
  }

  return true;
}

}
