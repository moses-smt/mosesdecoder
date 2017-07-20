// $Id$

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

#ifndef moses_LanguageModelReloading_h
#define moses_LanguageModelReloading_h

#include <string>

#include "moses/LM/Base.h"
#include "moses/LM/Ken.h"

#include "util/tokenize_piece.hh"
#include "util/string_stream.hh"

#include <iostream>
namespace Moses
{

class FFState;
template <class Model> class ReloadingLanguageModel : public LanguageModelKen<Model>
{
public:

  ReloadingLanguageModel(const std::string &line, const std::string &file, FactorType factorType, bool lazy) : LanguageModelKen<Model>(line, file, factorType, lazy ? util::LAZY : util::POPULATE_OR_READ), m_file(file), m_lazy(lazy) {

    VERBOSE(1, "ReloadingLM constructor: " << m_file << std::endl);

  }

  virtual void InitializeForInput(ttasksptr const& ttask) {
    VERBOSE(1, "ReloadingLM InitializeForInput" << std::endl);

    // The context scope object for this translation task
    //     contains a map of translation task-specific data
    boost::shared_ptr<Moses::ContextScope> contextScope = ttask->GetScope();

    // The key to the map is this object
    void const* key = static_cast<void const*>(this);

    // The value stored in the map is a string representing a phrase table
    boost::shared_ptr<string> value = contextScope->get<string>(key);

    // Create a stream to read the phrase table data
    stringstream strme(*(value.get()));

    ofstream tmp;
    tmp.open(m_file.c_str());

    // Read the phrase table data, one line at a time
    string line;
    while (getline(strme, line)) {

      tmp << line << "\n";

    }

    tmp.close();

    LanguageModelKen<Model>::LoadModel(m_file, m_lazy ? util::LAZY : util::POPULATE_OR_READ);
  };


protected:

  using LanguageModelKen<Model>::m_ngram;
  using LanguageModelKen<Model>::m_lmIdLookup;
  using LanguageModelKen<Model>::m_beginSentenceFactor;

  const std::string m_file;
  bool m_lazy;
};


LanguageModel *ConstructReloadingLM(const std::string &line, const std::string &file, FactorType factorType, bool lazy)
{
  lm::ngram::ModelType model_type;
  if (lm::ngram::RecognizeBinary(file.c_str(), model_type)) {
    switch(model_type) {
    case lm::ngram::PROBING:
      return new ReloadingLanguageModel<lm::ngram::ProbingModel>(line, file, factorType, lazy);
    case lm::ngram::REST_PROBING:
      return new ReloadingLanguageModel<lm::ngram::RestProbingModel>(line, file, factorType, lazy);
    case lm::ngram::TRIE:
      return new ReloadingLanguageModel<lm::ngram::TrieModel>(line, file, factorType, lazy);
    case lm::ngram::QUANT_TRIE:
      return new ReloadingLanguageModel<lm::ngram::QuantTrieModel>(line, file, factorType, lazy);
    case lm::ngram::ARRAY_TRIE:
      return new ReloadingLanguageModel<lm::ngram::ArrayTrieModel>(line, file, factorType, lazy);
    case lm::ngram::QUANT_ARRAY_TRIE:
      return new ReloadingLanguageModel<lm::ngram::QuantArrayTrieModel>(line, file, factorType, lazy);
    default:
      UTIL_THROW2("Unrecognized kenlm model type " << model_type);
    }
  } else {
    return new ReloadingLanguageModel<lm::ngram::ProbingModel>(line, file, factorType, lazy);
  }
}

LanguageModel *ConstructReloadingLM(const std::string &lineOrig)
{
  FactorType factorType = 0;
  std::string filePath;
  bool lazy = false;

  util::TokenIter<util::SingleCharacter, true> argument(lineOrig, ' ');
  ++argument; // KENLM

  util::StringStream line;
  line << "KENLM";

  for (; argument; ++argument) {
    const char *equals = std::find(argument->data(), argument->data() + argument->size(), '=');
    UTIL_THROW_IF2(equals == argument->data() + argument->size(),
                   "Expected = in ReloadingLM argument " << *argument);
    StringPiece name(argument->data(), equals - argument->data());
    StringPiece value(equals + 1, argument->data() + argument->size() - equals - 1);
    if (name == "factor") {
      factorType = boost::lexical_cast<FactorType>(value);
    } else if (name == "order") {
      // Ignored
    } else if (name == "path") {
      filePath.assign(value.data(), value.size());
    } else if (name == "lazyken") {
      lazy = boost::lexical_cast<bool>(value);
    } else {
      // pass to base class to interpret
      line << " " << name << "=" << value;
    }
  }

  return ConstructReloadingLM(line.str(), filePath, factorType, lazy);
}


} // namespace Moses

#endif

