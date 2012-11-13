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

void Tokenize(Phrase &phrase
              , const std::string &token, bool addSourceNonTerm, bool addTargetNonTerm
              , OnDiskWrapper &onDiskWrapper)
{

  bool nonTerm = false;
  size_t tokSize = token.size();
  int comStr =token.compare(0, 1, "[");

  if (comStr == 0) {
    comStr = token.compare(tokSize - 1, 1, "]");
    nonTerm = comStr == 0;
  }

  if (nonTerm) {
    // non-term
    size_t splitPos = token.find_first_of("[", 2);
    std::string wordStr = token.substr(0, splitPos);

    if (splitPos == std::string::npos) {
      // lhs - only 1 word
      WordPtr word (new Word());
      word->CreateFromString(wordStr, onDiskWrapper.GetVocab());
      phrase.AddWord(word);
    } else {
      // source & target non-terms
      if (addSourceNonTerm) {
        WordPtr word( new Word());
        word->CreateFromString(wordStr, onDiskWrapper.GetVocab());
        phrase.AddWord(word);
      }

      wordStr = token.substr(splitPos, tokSize - splitPos);
      if (addTargetNonTerm) {
        WordPtr word(new Word());
        word->CreateFromString(wordStr, onDiskWrapper.GetVocab());
        phrase.AddWord(word);
      }

    }
  } else {
    // term
    WordPtr word(new Word());
    word->CreateFromString(token, onDiskWrapper.GetVocab());
    phrase.AddWord(word);
  }
}

SourcePhrase Tokenize(const std::vector<std::string>& tokens, OnDiskWrapper &onDiskWrapper)
{
  SourcePhrase sourcePhrase;
  if (tokens.size() > 0){
    std::vector<std::string>::const_iterator token = tokens.begin();
    for (; token + 1 != tokens.end(); ++token){
      Tokenize(sourcePhrase, *token, true, true, onDiskWrapper);
    }
    // last position. LHS non-term
    Tokenize(sourcePhrase, *token, false, true, onDiskWrapper);
  }
  return sourcePhrase;
}

const PhraseNode* Query(const SourcePhrase& sourcePhrase, OnDiskWrapper& onDiskWrapper)
{
    const PhraseNode *node = &onDiskWrapper.GetRootSourceNode();
    assert(node);
    
    for (size_t pos = 0; pos < sourcePhrase.GetSize(); ++pos)
    {
      const Word &word = sourcePhrase.GetWord(pos);
      node = node->GetChild(word, onDiskWrapper);
      if (node == NULL)
      {
        break;
      }
    }
    return node;
}

}
