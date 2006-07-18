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

#include <cstdlib>
#include <iostream>
using std::ifstream;
#include <vector>
using std::vector;
#include "TypeDef.h"
#include "StaticData.h"
#include "WordDeletionTable.h"
using std::string;

void WordDeletionTable::Load(const string& filename, StaticData& staticData)
{
	std::cout << "in WordDeletionTable::Load()" << std::endl;
	ifstream infile(filename.c_str());
	if(!infile)
	{
		std::cerr << "WordDeletionTable::Load(): can't open '" << filename << "' for read; exiting" << std::endl;
		exit(-1);
	}
	
	//each line is of format PHRASE ||| DELETION_COST
	string line;
	while(getline(infile, line, '\n'))
	{
		vector<string> token = TokenizeMultiCharSeparator(line, "|||");
		//parse phrase
		Phrase sourcePhrase(Input);
		const std::vector<FactorType>& input = staticData.GetInputFactorOrder();
		sourcePhrase.CreateFromString(input, token[0], staticData.GetFactorCollection());
		//parse cost
		m_deletionCosts[sourcePhrase] = Scan<float>(token[1]);
		std::cout << "dtable entry: " << sourcePhrase << " -> " << m_deletionCosts[sourcePhrase] << std::endl;
	}
	infile.close();
}
