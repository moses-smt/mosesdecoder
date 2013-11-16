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
#ifndef EXTRACT_GHKM_ALIGNMENT_H_
#define EXTRACT_GHKM_ALIGNMENT_H_

#include <string>
#include <utility>
#include <vector>

namespace Moses
{
namespace GHKM
{

typedef std::vector<std::pair<int, int> > Alignment;

void ReadAlignment(const std::string &, Alignment &);

}  // namespace GHKM
}  // namespace Moses

#endif
