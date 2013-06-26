#pragma once
#include <string>
#include <vector>
#include "OnDiskWrapper.h"
#include "Phrase.h"
#include "SourcePhrase.h"
#include "Word.h"
#include "PhraseNode.h"


namespace OnDiskPt
{

class OnDiskQuery
{
private:
  OnDiskWrapper &m_wrapper;

public:

  OnDiskQuery(OnDiskWrapper &wrapper):m_wrapper(wrapper) {}

  void Tokenize(Phrase &phrase,
                const std::string &token,
                bool addSourceNonTerm,
                bool addTargetNonTerm);

  SourcePhrase Tokenize(const std::vector<std::string>& tokens);

  const PhraseNode *Query(const SourcePhrase& sourcePhrase);

  inline const PhraseNode *Query(const std::vector<std::string>& tokens) {
    return Query(Tokenize(tokens));
  }

};


}
