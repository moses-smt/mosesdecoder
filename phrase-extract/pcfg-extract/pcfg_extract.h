/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh

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
#ifndef PCFG_EXTRACT_PCFG_EXTRACT_H_
#define PCFG_EXTRACT_PCFG_EXTRACT_H_

#include "pcfg-common/tool.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PCFG
{

struct Options;

class PcfgExtract : public Tool
{
public:
  PcfgExtract() : Tool("pcfg-extract") {}
  virtual int Main(int, char *[]);
private:
  void ProcessOptions(int, char *[], Options &) const;
};

}  // namespace PCFG
}  // namespace Syntax
}  // namespace MosesTraining

#endif
