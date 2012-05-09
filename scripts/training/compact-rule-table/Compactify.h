#pragma once
#ifndef COMPACTIFY_H_
#define COMPACTIFY_H_

#include "NumberedSet.h"
#include "Tool.h"

#include <set>
#include <vector>

namespace moses {

struct Options;

// Tool for converting a rule table into a more compact format.
class Compactify : public Tool {
 public:
  Compactify() : Tool("compactify") {}
  virtual int main(int, char *[]);
 private:
  typedef unsigned int SymbolIDType;
  typedef unsigned int PhraseIDType;
  typedef unsigned int AlignmentSetIDType;
  typedef std::vector<std::string> StringPhrase;
  typedef std::vector<SymbolIDType> SymbolPhrase;
  typedef std::pair<int, int> AlignmentPair;
  typedef std::set<AlignmentPair> AlignmentSet;
  typedef NumberedSet<std::string, SymbolIDType> SymbolSet;
  typedef NumberedSet<SymbolPhrase, PhraseIDType> PhraseSet;
  typedef NumberedSet<AlignmentSet, AlignmentSetIDType> AlignmentSetSet;

  void processOptions(int, char *[], Options &) const;

  // Given the string representations of a source or target LHS and RHS, encode
  // the symbols using the given SymbolSet and create a SymbolPhrase object.
  // The LHS index is the first element of the SymbolPhrase.
  void encodePhrase(const std::string &, const StringPhrase &,
                    SymbolSet &, SymbolPhrase &) const;
};

}  // namespace moses

#endif
