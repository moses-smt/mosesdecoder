/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) University of Edinburgh

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

#include "PropertiesConsolidator.h"

#include <sstream>
#include <limits>
#include <vector>

#include "moses/Util.h"
#include "phrase-extract/InputFileStream.h"
#include "phrase-extract/OutputFileStream.h"


namespace MosesTraining
{

void PropertiesConsolidator::ActivateSourceLabelsProcessing(const std::string &sourceLabelSetFile)
{
  Moses::InputFileStream inFile(sourceLabelSetFile);

  // read source label set
  m_sourceLabels.clear();
  std::string line;
  while (getline(inFile, line)) {
    std::istringstream tokenizer(line);
    std::string label;
    size_t index;
    try {
      tokenizer >> label >> index;
    } catch (const std::exception &e) {
      UTIL_THROW2("Error reading source label set file " << sourceLabelSetFile << " .");
    }
    std::pair< std::map<std::string,size_t>::iterator, bool > inserted = m_sourceLabels.insert( std::pair<std::string,size_t>(label,index) );
    UTIL_THROW_IF2(!inserted.second,"Source label set file " << sourceLabelSetFile << " should contain each syntactic label only once.");
  }

  inFile.Close();

  m_sourceLabelsFlag = true;
}


void PropertiesConsolidator::ActivatePartsOfSpeechProcessing(const std::string &partsOfSpeechFile)
{
  Moses::InputFileStream inFile(partsOfSpeechFile);

  // read parts-of-speech vocabulary
  m_partsOfSpeechVocabulary.clear();
  std::string line;
  while (getline(inFile, line)) {
    std::istringstream tokenizer(line);
    std::string label;
    size_t index;
    try {
      tokenizer >> label >> index;
    } catch (const std::exception &e) {
      UTIL_THROW2("Error reading part-of-speech vocabulary file " << partsOfSpeechFile << " .");
    }
    std::pair< std::map<std::string,size_t>::iterator, bool > inserted = m_partsOfSpeechVocabulary.insert( std::pair<std::string,size_t>(label,index) );
    UTIL_THROW_IF2(!inserted.second,"Part-of-speech vocabulary file " << partsOfSpeechFile << " should contain each POS tag only once.");
  }

  inFile.Close();

  m_partsOfSpeechFlag = true;
}


void PropertiesConsolidator::ActivateTargetSyntacticPreferencesProcessing(const std::string &targetSyntacticPreferencesLabelSetFile)
{
  Moses::InputFileStream inFile(targetSyntacticPreferencesLabelSetFile);

  // read target syntactic preferences label set
  m_targetSyntacticPreferencesLabels.clear();
  std::string line;
  while (getline(inFile, line)) {
    std::istringstream tokenizer(line);
    std::string label;
    size_t index;
    try {
      tokenizer >> label >> index;
    } catch (const std::exception &e) {
      UTIL_THROW2("Error reading target syntactic preferences label set file " << targetSyntacticPreferencesLabelSetFile << " .");
    }
    std::pair< std::map<std::string,size_t>::iterator, bool > inserted = m_targetSyntacticPreferencesLabels.insert( std::pair<std::string,size_t>(label,index) );
    UTIL_THROW_IF2(!inserted.second,"Target syntactic preferences label set file " << targetSyntacticPreferencesLabelSetFile << " should contain each syntactic label only once.");
  }

  inFile.Close();

  m_targetSyntacticPreferencesFlag = true;
}


void PropertiesConsolidator::ProcessPropertiesString(const std::string &propertiesString, Moses::OutputFileStream& out) const
{
  if ( propertiesString.empty() ) {
    return;
  }

  std::vector<std::string> toks;
  Moses::TokenizeMultiCharSeparator(toks, propertiesString, "{{");
  for (size_t i = 1; i < toks.size(); ++i) {
    std::string &tok = toks[i];
    if (tok.empty()) {
      continue;
    }
    size_t endPos = tok.rfind("}");
    tok = tok.substr(0, endPos - 1);
    std::vector<std::string> keyValue = Moses::TokenizeFirstOnly(tok, " ");
    assert(keyValue.size() == 2);

    if ( !keyValue[0].compare("SourceLabels") ) {

      if ( m_sourceLabelsFlag ) {

        // SourceLabels property: replace strings with vocabulary indices
        out << " {{" << keyValue[0];
        ProcessSourceLabelsPropertyValue(keyValue[1], out);
        out << "}}";

      } else { // don't process SourceLabels property
        out << " {{" << keyValue[0] << " " << keyValue[1] << "}}";
      }

    } else if ( !keyValue[0].compare("POS") ) {

      /* DO NOTHING (property is not registered in the decoder at the moment)
            if ( m_partsOfSpeechFlag ) {

              // POS property: replace strings with vocabulary indices
              out << " {{" << keyValue[0];
              ProcessPOSPropertyValue(keyValue[1], out);
              out << "}}";

            } else { // don't process POS property
              out << " {{" << keyValue[0] << " " << keyValue[1] << "}}";
            }
      */

    } else if ( !keyValue[0].compare("TargetPreferences") ) {

      if ( m_targetSyntacticPreferencesFlag ) {

        // TargetPreferences property: replace strings with vocabulary indices
        out << " {{" << keyValue[0];
        ProcessTargetSyntacticPreferencesPropertyValue(keyValue[1], out);
        out << "}}";

      } else { // don't process TargetPreferences property
        out << " {{" << keyValue[0] << " " << keyValue[1] << "}}";
      }

    } else {

      // output other property
      out << " {{" << keyValue[0] << " " << keyValue[1] << "}}";
    }
  }
}


void PropertiesConsolidator::ProcessSourceLabelsPropertyValue(const std::string &value, Moses::OutputFileStream& out) const
{
  // SourceLabels property: replace strings with vocabulary indices
  std::istringstream tokenizer(value);

  size_t nNTs;
  double totalCount;

  if (! (tokenizer >> nNTs)) { // first token: number of non-terminals (incl. left-hand side)
    UTIL_THROW2("Not able to read number of non-terminals from SourceLabels property. "
                << "Flawed SourceLabels property?");
  }
  assert( nNTs > 0 );
  out << " " << nNTs;

  if (! (tokenizer >> totalCount)) { // second token: overall rule count
    UTIL_THROW2("Not able to read overall rule count from SourceLabels property. "
                << "Flawed SourceLabels property?");
  }
  assert( totalCount > 0.0 );
  out << " " << totalCount;

  while (tokenizer.peek() != EOF) {
    try {

      size_t numberOfLHSsGivenRHS = std::numeric_limits<std::size_t>::max();

      std::string token;

      if (nNTs > 1) { // rule has right-hand side non-terminals, i.e. it's a hierarchical rule
        for (size_t i=0; i<nNTs-1; ++i) { // RHS source non-terminal labels
          tokenizer >> token; // RHS source non-terminal label
          std::map<std::string,size_t>::const_iterator found = m_sourceLabels.find(token);
          UTIL_THROW_IF2(found == m_sourceLabels.end(), "Label \"" << token << "\" from the phrase table not found in given label set.");
          out << " " << found->second;
        }

        tokenizer >> token; // sourceLabelsRHSCount
        out << " " << token;

        tokenizer >> numberOfLHSsGivenRHS;
        out << " " << numberOfLHSsGivenRHS;
      }

      for (size_t i=0; i<numberOfLHSsGivenRHS && tokenizer.peek()!=EOF; ++i) { // LHS source non-terminal labels seen with this RHS
        tokenizer >> token; // LHS source non-terminal label
        std::map<std::string,size_t>::const_iterator found = m_sourceLabels.find(token);
        UTIL_THROW_IF2(found == m_sourceLabels.end() ,"Label \"" << token << "\" from the phrase table not found in given label set.");
        out << " " << found->second;

        tokenizer >> token; // ruleSourceLabelledCount
        out << " " << token;
      }

    } catch (const std::exception &e) {
      UTIL_THROW2("Flawed item in SourceLabels property?");
    }
  }
}


void PropertiesConsolidator::ProcessPOSPropertyValue(const std::string &value, Moses::OutputFileStream& out) const
{
  std::istringstream tokenizer(value);
  while (tokenizer.peek() != EOF) {
    std::string token;
    tokenizer >> token;
    std::map<std::string,size_t>::const_iterator found = m_partsOfSpeechVocabulary.find(token);
    UTIL_THROW_IF2(found == m_partsOfSpeechVocabulary.end() ,"Part-of-speech \"" << token << "\" from the phrase table not found in given part-of-speech vocabulary.");
    out << " " << found->second;
  }
}


bool PropertiesConsolidator::GetPOSPropertyValueFromPropertiesString(const std::string &propertiesString, std::vector<std::string>& out) const
{
  out.clear();
  if ( propertiesString.empty() ) {
    return false;
  }

  std::vector<std::string> toks;
  Moses::TokenizeMultiCharSeparator(toks, propertiesString, "{{");
  for (size_t i = 1; i < toks.size(); ++i) {
    std::string &tok = toks[i];
    if (tok.empty()) {
      continue;
    }
    size_t endPos = tok.rfind("}");
    tok = tok.substr(0, endPos - 1);
    std::vector<std::string> keyValue = Moses::TokenizeFirstOnly(tok, " ");
    assert(keyValue.size() == 2);

    if ( !keyValue[0].compare("POS") ) {
      std::istringstream tokenizer(keyValue[1]);
      while (tokenizer.peek() != EOF) {
        std::string token;
        tokenizer >> token;
        out.push_back(token);
      }
      return true;
    }
  }

  return false;
}


void PropertiesConsolidator::ProcessTargetSyntacticPreferencesPropertyValue(const std::string &value, Moses::OutputFileStream& out) const
{
  // TargetPreferences property: replace strings with vocabulary indices
  std::istringstream tokenizer(value);

  size_t nNTs;
  double totalCount;

  if (! (tokenizer >> nNTs)) { // first token: number of non-terminals (incl. left-hand side)
    UTIL_THROW2("Not able to read number of non-terminals from TargetPreferences property. "
                << "Flawed TargetPreferences property?");
  }
  assert( nNTs > 0 );
  out << " " << nNTs;

  if (! (tokenizer >> totalCount)) { // second token: overall rule count
    UTIL_THROW2("Not able to read overall rule count from TargetPreferences property. "
                << "Flawed TargetPreferences property?");
  }
  assert( totalCount > 0.0 );
  out << " " << totalCount;

  while (tokenizer.peek() != EOF) {
    try {

      size_t numberOfLHSsGivenRHS = std::numeric_limits<std::size_t>::max();

      std::string token;

      if (nNTs > 1) { // rule has right-hand side non-terminals, i.e. it's a hierarchical rule
        for (size_t i=0; i<nNTs-1; ++i) { // RHS target preference non-terminal labels
          tokenizer >> token; // RHS target preference non-terminal label
          std::map<std::string,size_t>::const_iterator found = m_targetSyntacticPreferencesLabels.find(token);
          UTIL_THROW_IF2(found == m_targetSyntacticPreferencesLabels.end(), "Label \"" << token << "\" from the phrase table not found in given label set.");
          out << " " << found->second;
        }

        tokenizer >> token; // targetPreferenceRHSCount
        out << " " << token;

        tokenizer >> numberOfLHSsGivenRHS;
        out << " " << numberOfLHSsGivenRHS;
      }

      for (size_t i=0; i<numberOfLHSsGivenRHS && tokenizer.peek()!=EOF; ++i) { // LHS target preference non-terminal labels seen with this RHS
        tokenizer >> token; // LHS target preference non-terminal label
        std::map<std::string,size_t>::const_iterator found = m_targetSyntacticPreferencesLabels.find(token);
        UTIL_THROW_IF2(found == m_targetSyntacticPreferencesLabels.end() ,"Label \"" << token << "\" from the phrase table not found in given label set.");
        out << " " << found->second;

        tokenizer >> token; // ruleTargetPreferenceLabelledCount
        out << " " << token;
      }

    } catch (const std::exception &e) {
      UTIL_THROW2("Flawed item in TargetPreferences property?");
    }
  }
}


}  // namespace MosesTraining

