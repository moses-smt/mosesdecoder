//
//  WordLattice2.h
//  moses
//
//  Created by Hieu Hoang on 30/10/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef moses_WordLattice2_h
#define moses_WordLattice2_h

#include "InputType.h"

namespace Moses
{

class WordLattice2 : public InputType
{

public:
  InputTypeEnum GetType() const 
  { return WordLatticeInput2; }
  
  virtual size_t GetSize() const;
  
  //! populate this InputType with data from in stream
  virtual int Read(std::istream& in,const std::vector<FactorType>& factorOrder);

  //! Output debugging info to stream out
  virtual void Print(std::ostream&) const;
  
  //! create trans options specific to this InputType
  virtual TranslationOptionCollection* CreateTranslationOptionCollection(const TranslationSystem* system) const;
  
  //! return substring. Only valid for Sentence class. TODO - get rid of this fn
  virtual Phrase GetSubString(const WordsRange&) const;
  
  //! return substring at a particular position. Only valid for Sentence class. TODO - get rid of this fn
  virtual const Word& GetWord(size_t pos) const;

  virtual const NonTerminalSet &GetLabelSet(size_t startPos, size_t endPos) const;

  
};

}

#endif
