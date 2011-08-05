/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2010 University of Edinburgh

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

////////////////////////////////////////////////////////////////////////////////
//
//  extract-ghkm
//  SCFG grammar rule extractor based on the GHKM algorithm described in:
//
//    Galley, M., Hopkins, M., Knight, K., and Marcu, D. (2004)
//    "What's in a Translation Rule?", In Proceedings of HLT/NAACL 2004.
//
////////////////////////////////////////////////////////////////////////////////

#include "Alignment.h"
#include "AlignmentGraph.h"
#include "Exception.h"
#include "ParseTree.h"
#include "Rule.h"
#include "Span.h"
#include "XmlTreeParser.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

namespace
{
const std::string progName = "extract-ghkm";

void
printUsage()
{
  std::cerr << "Usage: "
            << progName << " TARGET SOURCE ALIGNMENT EXTRACT"
            << std::endl;
}

void
printErrorMsg(const std::string & errorMsg)
{
  std::cerr << progName << ": " << errorMsg << std::endl;
}

void
printSymbol(const Symbol & symbol, std::ostream & out)
{
  if (symbol.getType() == NonTerminal) {
    out << "[" << symbol.getValue() << "]";
  } else {
    out << symbol.getValue();
  }
}

void
printRule(const Rule & rule, std::ostream & out, std::ostream & invOut)
{
  const std::vector<Symbol> & sourceRHS = rule.getSourceRHS();
  const std::vector<Symbol> & targetRHS = rule.getTargetRHS();

  // TODO Just create maps for NTs (one-to-one)
  std::map<int, std::vector<int> > sourceToTarget;
  std::map<int, std::vector<int> > targetToSource;

  const Alignment & alignment = rule.getAlignment();

  for (Alignment::const_iterator p(alignment.begin());
       p != alignment.end(); ++p) {
    sourceToTarget[p->first].push_back(p->second);
    targetToSource[p->second].push_back(p->first);
  }

  std::ostringstream sourceSS;
  std::ostringstream targetSS;

  int i = 0;
  for (std::vector<Symbol>::const_iterator p(sourceRHS.begin());
       p != sourceRHS.end(); ++p, ++i) {
    printSymbol(*p, sourceSS);
    if (p->getType() == NonTerminal) {
      assert(sourceToTarget.find(i) != sourceToTarget.end());
      const std::vector<int> & targetIndices = sourceToTarget[i];
      assert(targetIndices.size() == 1);
      int targetIndex = targetIndices[0];
      printSymbol(targetRHS[targetIndex], sourceSS);
    }
    sourceSS << " ";
  }
  printSymbol(rule.getSourceLHS(), sourceSS);

  i = 0;
  for (std::vector<Symbol>::const_iterator p(targetRHS.begin());
       p != targetRHS.end(); ++p, ++i) {
    if (p->getType() == NonTerminal) {
      assert(targetToSource.find(i) != targetToSource.end());
      const std::vector<int> & sourceIndices = targetToSource[i];
      assert(sourceIndices.size() == 1);
      int sourceIndex = sourceIndices[0];
      printSymbol(sourceRHS[sourceIndex], targetSS);
    }
    printSymbol(*p, targetSS);
    targetSS << " ";
  }

  printSymbol(rule.getTargetLHS(), targetSS);

  out << sourceSS.str() << " ||| " << targetSS.str() << " |||";
  invOut << targetSS.str() << " ||| " << sourceSS.str() << " |||";

  for (Alignment::const_iterator p(alignment.begin());
       p != alignment.end(); ++p) {
    out << " " << p->first << "-" << p->second;
    invOut << " " << p->second << "-" << p->first;
  }

  out << " ||| 1" << std::endl;
  invOut << " ||| 1" << std::endl;
}

std::vector<std::string>
readTokens(const std::string & s)
{
  std::vector<std::string> tokens;

  std::string whitespace = " \t";

  std::string::size_type begin = s.find_first_not_of(whitespace);
  assert(begin != std::string::npos);
  while (true) {
    std::string::size_type end = s.find_first_of(whitespace, begin);
    std::string token;
    if (end == std::string::npos) {
      token = s.substr(begin);
    } else {
      token = s.substr(begin, end-begin);
    }
    tokens.push_back(token);
    if (end == std::string::npos) {
      break;
    }
    begin = s.find_first_not_of(whitespace, end);
    if (begin == std::string::npos) {
      break;
    }
  }

  return tokens;
}
}

int
main(int argc, char * argv[])
{
  if (argc != 5) {
    printUsage();
    exit(1);
  }

  std::ifstream targetStream(argv[1]);
  if (!targetStream) {
    printErrorMsg("Failed to open file: " + std::string(argv[1]));
    exit(1);
  }

  std::ifstream sourceStream(argv[2]);
  if (!sourceStream) {
    printErrorMsg("Failed to open file: " + std::string(argv[2]));
    exit(1);
  }

  std::ifstream alignmentStream(argv[3]);
  if (!alignmentStream) {
    printErrorMsg("Failed to open file: " + std::string(argv[3]));
    exit(1);
  }

  std::ofstream extractStream(argv[4]);
  if (!extractStream) {
    printErrorMsg("Failed to open file: " + std::string(argv[4]));
    exit(1);
  }

  std::string invExtractFileName = std::string(argv[4]) + std::string(".inv");
  std::ofstream invExtractStream(invExtractFileName.c_str());
  if (!invExtractStream) {
    printErrorMsg("Failed to open file: " + invExtractFileName);
    exit(1);
  }

  size_t lineNum = 0;
  while (true) {
    std::string targetLine;
    std::getline(targetStream, targetLine);

    std::string sourceLine;
    std::getline(sourceStream, sourceLine);

    std::string alignmentLine;
    std::getline(alignmentStream, alignmentLine);

    if (targetStream.eof() && sourceStream.eof() && alignmentStream.eof()) {
      break;
    }

    if (targetStream.eof() || sourceStream.eof() || alignmentStream.eof()) {
      printErrorMsg("Files must contain same number of lines");
      exit(1);
    }

    ++lineNum;

    std::auto_ptr<ParseTree> t(parseXmlTree(targetLine));
    if (!t.get()) {
      std::ostringstream s;
      s << "Failed to parse XML tree at line " << lineNum;
      printErrorMsg(s.str());
      exit(1);
    }

    std::vector<std::string> sourceTokens(readTokens(sourceLine));

    Alignment alignment;
    try {
      alignment = readAlignment(alignmentLine);
    } catch (const Exception & e) {
      std::ostringstream s;
      s << "Failed to read alignment at line " << lineNum << ": ";
      s << e.getMsg();
      printErrorMsg(s.str());
      exit(1);
    }

    AlignmentGraph graph(t.get(), sourceTokens, alignment);

    std::vector<Rule> rules(graph.inferRules());

    for (std::vector<Rule>::iterator p(rules.begin());
         p != rules.end(); ++p) {
      printRule(*p, extractStream, invExtractStream);
    }
  }

  return 0;
}
