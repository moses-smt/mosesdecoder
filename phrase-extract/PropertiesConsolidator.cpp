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


std::string PropertiesConsolidator::ProcessPropertiesString(const std::string &propertiesString) const
{
  if ( propertiesString.empty() ) {
    return propertiesString;
  }

  std::ostringstream out;
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

        // SourceLabels additional property: replace strings with vocabulary indices
        out << " {{" << keyValue[0];

        std::istringstream tokenizer(keyValue[1]);

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

        out << "}}";

      } else { // don't process source labels additional property
        out << " {{" << keyValue[0] << " " << keyValue[1] << "}}";
      }

    } else {

      // output other additional property
      out << " {{" << keyValue[0] << " " << keyValue[1] << "}}";
    }
  }

  return out.str();
}

}  // namespace MosesTraining

