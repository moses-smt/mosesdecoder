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

#include "Loader.h"

namespace Moses
{

//! Loader to load Moses-formatted SCFG rules from a text file
class RuleTableLoaderStandard : public RuleTableLoader
{
protected:

  bool Load(FormatType format,
            const std::vector<FactorType> &input,
            const std::vector<FactorType> &output,
            const std::string &inFile,
            size_t tableLimit,
            RuleTableTrie &);
public:
  bool Load(const std::vector<FactorType> &input,
            const std::vector<FactorType> &output,
            const std::string &inFile,
            size_t tableLimit,
            RuleTableTrie &);
};

}  // namespace Moses
