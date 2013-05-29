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

#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/StaticData.h"
#include "moses/InputType.h"
#include "moses/TranslationOption.h"
#include "moses/UserMessage.h"

using namespace std;

namespace Moses
{

PhraseDictionary::PhraseDictionary(const std::string &description, const std::string &line)
  :DecodeFeature(description, line)
{
  m_tableLimit= 20; // TODO default?

  for (size_t i = 0; i < m_args.size(); ++i) {
    const vector<string> &args = m_args[i];

    if (args[0] == "num-input-features") {
      m_numInputScores = Scan<unsigned>(args[1]);
    } else if (args[0] == "path") {
      m_filePath = args[1];
    } else if (args[0] == "table-limit") {
      m_tableLimit = Scan<size_t>(args[1]);
    } else if (args[0] == "target-path") {
      m_targetFile = args[1];
    } else if (args[0] == "alignment-path") {
      m_alignmentsFile = args[1];
    } else {
      //throw "Unknown argument " + args[0];
    }
  } // for (size_t i = 0; i < toks.size(); ++i) {

}


const TargetPhraseCollection *PhraseDictionary::
GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const
{
  Phrase phrase = src.GetSubString(range);
  phrase.OnlyTheseFactors(m_inputFactors);
  return GetTargetPhraseCollection(phrase);
}

}

