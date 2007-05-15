// $Id: StaticData.h 1338 2007-04-06 00:24:25Z hieuhoang1972 $

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

#pragma once

std::vector<const Factor*> doMBR(const LatticePathList& nBestList);
void GetOutputFactors(const LatticePath &path, std::vector <const Factor*> &translation);
double calculate_score(const std::vector< std::vector<const Factor*> > & sents, int ref, int hyp,  std::vector < std::map < std::vector < const Factor *>, int > > & ngram_stats );

