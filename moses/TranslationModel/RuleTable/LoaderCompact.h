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

#pragma once

#include "moses/Phrase.h"
#include "moses/Word.h"
#include "moses/TypeDef.h"
#include "Loader.h"

#include <istream>
#include <string>
#include <vector>

namespace Moses
{
class RuleTableTrie;

//! @todo ask phil williams
class RuleTableLoaderCompact : public RuleTableLoader
{
public:
  bool Load(AllOptions const& opts,
            const std::vector<FactorType> &input,
            const std::vector<FactorType> &output,
            const std::string &inFile,
            size_t tableLimit,
            RuleTableTrie &);

private:
  struct LineReader {
    LineReader(std::istream &input) : m_input(input), m_lineNum(0) {}
    void ReadLine() {
      std::getline(m_input, m_line);
      // Assume everything's hunky-dory.
      ++m_lineNum;
    }
    std::istream &m_input;
    std::string m_line;
    size_t m_lineNum;
  };

  void LoadVocabularySection(LineReader &,
                             const std::vector<FactorType> &,
                             std::vector<Word> &);

  void LoadPhraseSection(LineReader &,
                         const std::vector<Word> &,
                         std::vector<Phrase> &,
                         std::vector<size_t> &);

  void LoadAlignmentSection(LineReader &,
                            std::vector<const AlignmentInfo *> &,
                            std::vector<Phrase> &);

  bool LoadRuleSection(LineReader &,
                       const std::vector<Word> &,
                       const std::vector<Phrase> &,
                       const std::vector<Phrase> &,
                       const std::vector<size_t> &,
                       const std::vector<const AlignmentInfo *> &,
                       RuleTableTrie &ruleTable);

  // Like Tokenize() but records starting positions of tokens (instead of
  // copying substrings) and assumes delimiter is ASCII space character.
  void FindTokens(std::vector<size_t> &output, const std::string &str) const {
    // Skip delimiters at beginning.
    size_t lastPos = str.find_first_not_of(' ', 0);
    // Find first "non-delimiter".
    size_t pos = str.find_first_of(' ', lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos) {
      // Found a token, add it to the vector.
      output.push_back(lastPos);
      // Skip delimiters.  Note the "not_of"
      lastPos = str.find_first_not_of(' ', pos);
      // Find next "non-delimiter"
      pos = str.find_first_of(' ', lastPos);
    }
  }
};

}  // namespace Moses
