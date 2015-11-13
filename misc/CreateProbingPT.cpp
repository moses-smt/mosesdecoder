#include "util/usage.hh"
#include "moses/TranslationModel/ProbingPT/storing.hh"



int main(int argc, char* argv[])
{

  const char * is_reordering = "false";

  if (!(argc == 5 || argc == 4)) {
    // Tell the user how to run the program
    std::cerr << "Provided " << argc << " arguments, needed 4 or 5." << std::endl;
    std::cerr << "Usage: " << argv[0] << " path_to_phrasetable output_dir num_scores is_reordering" << std::endl;
    std::cerr << "is_reordering should be either true or false, but it is currently a stub feature." << std::endl;
    //std::cerr << "Usage: " << argv[0] << " path_to_phrasetable number_of_uniq_lines output_bin_file output_hash_table output_vocab_id" << std::endl;
    return 1;
  }

  if (argc == 5) {
    is_reordering = argv[4];
  }

  createProbingPT(argv[1], argv[2], argv[3], is_reordering);

  util::PrintUsage(std::cout);
  return 0;
}

