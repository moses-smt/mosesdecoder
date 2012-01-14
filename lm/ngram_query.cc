#include "lm/ngram_query.hh"

int main(int argc, char *argv[]) {
  if (!(argc == 2 || (argc == 3 && !strcmp(argv[2], "null")))) {
    std::cerr << "Usage: " << argv[0] << " lm_file [null]" << std::endl;
    std::cerr << "Input is wrapped in <s> and </s> unless null is passed." << std::endl;
    return 1;
  }
  try {
    bool sentence_context = (argc == 2);
    lm::ngram::ModelType model_type;
    if (lm::ngram::RecognizeBinary(argv[1], model_type)) {
      switch(model_type) {
        case lm::ngram::HASH_PROBING:
          Query<lm::ngram::ProbingModel>(argv[1], sentence_context, std::cin, std::cout);
          break;
        case lm::ngram::TRIE_SORTED:
          Query<lm::ngram::TrieModel>(argv[1], sentence_context, std::cin, std::cout);
          break;
        case lm::ngram::QUANT_TRIE_SORTED:
          Query<lm::ngram::QuantTrieModel>(argv[1], sentence_context, std::cin, std::cout);
          break;
        case lm::ngram::ARRAY_TRIE_SORTED:
          Query<lm::ngram::ArrayTrieModel>(argv[1], sentence_context, std::cin, std::cout);
          break;
        case lm::ngram::QUANT_ARRAY_TRIE_SORTED:
          Query<lm::ngram::QuantArrayTrieModel>(argv[1], sentence_context, std::cin, std::cout);
          break;
        case lm::ngram::HASH_SORTED:
        default:
          std::cerr << "Unrecognized kenlm model type " << model_type << std::endl;
          abort();
      }
    } else {
      Query<lm::ngram::ProbingModel>(argv[1], sentence_context, std::cin, std::cout);
    }

    PrintUsage("Total time including destruction:\n");
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}
