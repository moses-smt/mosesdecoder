#include "util/usage.hh"
#include "moses/TranslationModel/ProbingPT/storing.hh"



int main(int argc, char* argv[]){

    if (argc != 3) {
        // Tell the user how to run the program
        std::cerr << "Provided " << argc << " arguments, needed 3." << std::endl;
        std::cerr << "Usage: " << argv[0] << " path_to_phrasetable output_dir" << std::endl;
        return 1;
    }

    createProbingPT(argv[1], argv[2]);

    util::PrintUsage(std::cout);
    return 0;
}

