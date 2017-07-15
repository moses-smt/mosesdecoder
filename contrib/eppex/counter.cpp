/**
 * Epochal Phrase Extraction - extracted phrase pairs counter. Run this program
 * the same way as phrase-extract/extract (without output files). Numbers of
 * extracted phrase pairs for each length up to max-phrase-length will be
 * reported.
 *
 * (C) Moses: http://www.statmt.org/moses/
 * (C) Ceslav Przywara, UFAL MFF UK, 2011
 *
 * $Id$
 */

#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <iomanip>

#include "phrase-extract.h"
#include "shared.h"

#define REQUIRED_PARAMS_NUM 4


void usage(const char* programName) {
    std::cerr << "syntax: " << std::string(programName) << " tgt src align max-phrase-length [orientation [ --model [wbe|phrase|hier]-[msd|mslr|mono] ]]]\n";
    exit(1);
}


/*******************************************************************************
 * MAIN                                                                        *
 ******************************************************************************/
int main(int argc, char** argv) {

    if (argc <= REQUIRED_PARAMS_NUM) {
        usage(argv[0]);
    }

    const char* fileNameE = argv[1];
    const char* fileNameF = argv[2];
    const char* fileNameA = argv[3];
    maxPhraseLength = std::atoi(argv[4]);

    //
    read_optional_params(argc, argv, 5);

    // Init phrase pairs counters (add +1 for dummy zero-length counter).
    phrasePairsCounters.resize(maxPhraseLength + 1, 0);

    std::cerr << "Starting phrase pairs counter ..." << std::endl;

    // open input files
    std::ifstream eFile(fileNameE);
    std::ifstream fFile(fileNameF);
    std::ifstream aFile(fileNameA);

    //
    readInput(eFile, fFile, aFile);

    std::cerr << std::endl; // Leave the progress bar end on previous line.

    // close input files
    eFile.close();
    fFile.close();
    aFile.close();

    std::cout
        << "############################" << std::endl
        << "# len # phrase pairs count #" << std::endl
        << "############################" << std::endl;

    for ( size_t i = 1; i < phrasePairsCounters.size(); ++i ) {
        std::cout << "# " << std::setw(3) << i << " # " << std::setw(18) << phrasePairsCounters[i] << " #" << std::endl;
    }

    std::cout
        << "############################" << std::endl;

}

