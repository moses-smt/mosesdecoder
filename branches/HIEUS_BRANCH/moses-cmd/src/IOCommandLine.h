// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (c) 2006 University of Edinburgh
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
			this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, 
			this list of conditions and the following disclaimer in the documentation 
			and/or other materials provided with the distribution.
    * Neither the name of the University of Edinburgh nor the names of its contributors 
			may be used to endorse or promote products derived from this software 
			without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS 
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

// example file on how to use moses library

#pragma once

#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "TypeDef.h"
#include "InputOutput.h"
#include "Sentence.h"

class FactorTypeSet;
class FactorCollection;

class IOCommandLine : public InputOutput
{
protected:
	const std::vector<FactorType>	&m_factorOrder;
	const FactorTypeSet						&m_inputFactorUsed;
	FactorCollection							&m_factorCollection;
	std::ofstream 								m_nBestFile;
public:
	IOCommandLine(const std::vector<FactorType>	&factorOrder
				, const FactorTypeSet						&inputFactorUsed
				, FactorCollection							&factorCollection
				, size_t												nBestSize
				, const std::string							&nBestFilePath);

	Sentence *GetInput();
	void SetOutput(const Hypothesis *hypo, long translationId);
	void SetNBest(const LatticePathList &nBestList, long translationId);
	void Backtrack(const Hypothesis *hypo);
};


// help fn
inline Sentence *GetInput(std::istream &inputStream
									 , const std::vector<FactorType> &factorOrder
									 , FactorCollection &factorCollection)
{
	std::string line;
	do 
	{
		if (getline(inputStream, line, '\n').eof())
		{
			return NULL;
		}
		boost::trim(line);
	} while (line == "");

	Sentence *sentence = new Sentence(Input);
	sentence->CreateFromString(factorOrder, line, factorCollection);
	return sentence;
}


