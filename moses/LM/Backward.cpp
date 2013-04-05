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

#include "lm/binary_format.hh"
#include "lm/enumerate_vocab.hh"
#include "lm/left.hh"
#include "lm/model.hh"

#include "moses/LM/Ken.h"
#include "moses/LM/Backward.h"

namespace Moses {

  template <class Model> BackwardLanguageModel<Model>::BackwardLanguageModel(const std::string &file, FactorType factorType, bool lazy) : LanguageModelKen<Model>(file,factorType,lazy) {
    //
    // This space intentionally left blank
    //
  }


  LanguageModel *ConstructBackwardLM(const std::string &file, FactorType factorType, bool lazy) {
    try {
      lm::ngram::ModelType model_type;
      if (lm::ngram::RecognizeBinary(file.c_str(), model_type)) {
	switch(model_type) {
        case lm::ngram::PROBING:
          return new BackwardLanguageModel<lm::ngram::ProbingModel>(file,  factorType, lazy);
        case lm::ngram::REST_PROBING:
          return new BackwardLanguageModel<lm::ngram::RestProbingModel>(file, factorType, lazy);
        case lm::ngram::TRIE:
          return new BackwardLanguageModel<lm::ngram::TrieModel>(file, factorType, lazy);
        case lm::ngram::QUANT_TRIE:
          return new BackwardLanguageModel<lm::ngram::QuantTrieModel>(file, factorType, lazy);
        case lm::ngram::ARRAY_TRIE:
          return new BackwardLanguageModel<lm::ngram::ArrayTrieModel>(file, factorType, lazy);
        case lm::ngram::QUANT_ARRAY_TRIE:
          return new BackwardLanguageModel<lm::ngram::QuantArrayTrieModel>(file, factorType, lazy);
        default:
          std::cerr << "Unrecognized kenlm model type " << model_type << std::endl;
          abort();
	}
      } else {
	return new BackwardLanguageModel<lm::ngram::ProbingModel>(file, factorType, lazy);
      }
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
      abort();
    }
  }

} // namespace Moses
