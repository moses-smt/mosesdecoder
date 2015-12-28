#include "util/usage.hh"
#include "moses/TranslationModel/ProbingPT/storing.hh"



int main(int argc, char* argv[])
{
  if (!(argc == 5 || argc == 4)) {
    // Tell the user how to run the program
    std::cerr << "Provided " << argc << " arguments, needed 4 or 5." << std::endl;
    std::cerr << "Usage: " << argv[0] << " path_to_phrasetable output_dir num_scores num_reordering_scores" << std::endl;
    //std::cerr << "Usage: " << argv[0] << " path_to_phrasetable number_of_uniq_lines output_bin_file output_hash_table output_vocab_id" << std::endl;
    return 1;
  }

	int num_lex_scores = 0;
	if (argc > 4) {
		num_lex_scores = atoi(argv[4]);
  } 

  createProbingPT(argv[1], argv[2], argv[3], num_lex_scores);

  util::PrintUsage(std::cout);
  return 0;
}

