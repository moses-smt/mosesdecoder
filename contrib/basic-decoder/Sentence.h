
#pragma once

#include "Phrase.h"

class Sentence :public Phrase
{
public:
  static Sentence *CreateFromString(const std::string &line);

  Sentence(size_t size);
  virtual ~Sentence();
};

