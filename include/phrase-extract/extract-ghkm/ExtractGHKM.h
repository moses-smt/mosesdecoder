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
#ifndef EXTRACT_GHKM_EXTRACT_GHKM_H_
#define EXTRACT_GHKM_EXTRACT_GHKM_H_

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

namespace Moses
{

class OutputFileStream;

namespace GHKM
{

struct Options;
class ParseTree;

class ExtractGHKM
{
public:
  ExtractGHKM() : m_name("extract-ghkm") {}
  const std::string &GetName() const {
    return m_name;
  }
  int Main(int argc, char *argv[]);
private:
  void Error(const std::string &) const;
  void OpenInputFileOrDie(const std::string &, std::ifstream &);
  void OpenOutputFileOrDie(const std::string &, std::ofstream &);
  void OpenOutputFileOrDie(const std::string &, OutputFileStream &);
  void RecordTreeLabels(const ParseTree &, std::set<std::string> &);
  void CollectWordLabelCounts(ParseTree &,
                              const Options &,
                              std::map<std::string, int> &,
                              std::map<std::string, std::string> &);
  void WriteUnknownWordLabel(const std::map<std::string, int> &,
                             const std::map<std::string, std::string> &,
                             const Options &,
                             std::ostream &);
  void WriteGlueGrammar(const std::set<std::string> &,
                        const std::map<std::string, int> &,
                        std::ostream &);
  std::vector<std::string> ReadTokens(const std::string &);

  void ProcessOptions(int, char *[], Options &) const;

  std::string m_name;
};

}  // namespace GHKM
}  // namespace Moses

#endif
