//
//  WordLattice2.cpp
//  moses
//
//  Created by Hieu Hoang on 30/10/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "WordLattice2.h"
#include "PCNTools.h"

using namespace std;

namespace Moses
{
size_t WordLattice2::GetSize() const
{
  
}

int WordLattice2::Read(std::istream& in,const std::vector<FactorType>& factorOrder)
{
  std::string line;
  if(!getline(in,line)) return 0;
  
  PCN::parseLattice(line);
  
}

void WordLattice2::Print(std::ostream&) const
{
  
}

//! create trans options specific to this InputType
TranslationOptionCollection* WordLattice2::CreateTranslationOptionCollection(const TranslationSystem* system) const
{
  
}

//! return substring. Only valid for Sentence class. TODO - get rid of this fn
Phrase WordLattice2::GetSubString(const WordsRange&) const
{
  
}

//! return substring at a particular position. Only valid for Sentence class. TODO - get rid of this fn
const Word& WordLattice2::GetWord(size_t pos) const
{
  
}

const NonTerminalSet &WordLattice2::GetLabelSet(size_t startPos, size_t endPos) const
{
  
}

}

