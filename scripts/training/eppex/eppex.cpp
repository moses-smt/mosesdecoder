/**
 * Epochal Phrase Extraction.
 *
 * (C) Moses: http://www.statmt.org/moses/
 * (C) Ceslav Przywara, UFAL MFF UK, 2011
 *
 * $Id$
 */


#include <string>
#include <iostream>
#include <fstream>
#include <string.h>

#include "config.h"
#include "phrase-extract.h"
#include "shared.h"


#define REQUIRED_PARAMS_NUM 5


//// Output processor declaration.

class FlushingOutputProcessor: public OutputProcessor {

private:
    const bool _compactOutputFlag;

public:
    FlushingOutputProcessor(bool compactOutputFlag): _compactOutputFlag(compactOutputFlag) {}

    void operator() (const std::string& srcPhrase, const std::string& tgtPhrase, const std::string& orientationInfo, const alignment_t& alignment, const size_t frequency, int mode);
};


//// Global variables.

// output files
std::ofstream extractFile; // extract
std::ofstream extractFileInv; // extract.inv
std::ofstream extractFileOrientation; // extract.o

////
bool compactOutputFlag = false; // Generate compact output:
// Each phrase pair is printed only once with its frequency prepended.
// Note that compacted output is not compatible with std phrase-extract format.


//// Functions.

void program_info(void) {
    std::cerr
        << "Epochal Phrase Extraction (" << PACKAGE_STRING << ") written by Ceslav Przywara (based on PhraseExtract v1.4 by Philipp Koehn).\n"
        << "Compiled with "
#ifdef USE_UNORDERED_MAP
        << "std::tr1::unordered_map"
#else
        << "std::map"
#endif
        << " implementation.\n"
	;
}

void read_optional_params(int argc, char* argv[], int optionalParamsStart);

void usage(const char* programName) {
    std::cerr << std::endl << "Syntax: " << std::string(programName) << " tgt src align extract lossy-counter [lossy-counter-2 [...]] [--compact] [--sort] [orientation [ --model [wbe|phrase|hier]-[msd|mslr|mono] ]]" << std::endl;
    std::cerr << get_lossy_counting_params_format();
    exit(1);
}


/*******************************************************************************
 * MAIN                                                                        *
 ******************************************************************************/
int main(int argc, char* argv[]) {

    // Welcome user with program info!
    program_info();

    if (argc <= REQUIRED_PARAMS_NUM) {
        usage(argv[0]);
    }

    const char* fileNameE = argv[1];
    const char* fileNameF = argv[2];
    const char* fileNameA = argv[3];
    std::string fileNameExtract = std::string(argv[4]);

    // Init lossy counters.
    std::string lossyCountersParams;
    int paramIdx = 5;
    
    while ( (argc > paramIdx) && (*argv[paramIdx] != '-') ) {
        std::string param = std::string(argv[paramIdx]);
        if ( !parse_lossy_counting_params(param) ) {
            usage(argv[0]);
        }
        lossyCountersParams += (" " + param);
        ++paramIdx;
    }

    if ( paramIdx == REQUIRED_PARAMS_NUM ) {
        std::cerr << "ERROR: no Lossy Counting parameters specified!" << std::endl;
        usage(argv[0]);
    }

    for ( size_t i = 1; i < lossyCounters.size(); ++i ) {
        if ( lossyCounters[i] == NULL ) {
            std::cerr << "ERROR: max phrase length set to " << maxPhraseLength << ", but no Lossy Counting parameters specified for phrase pairs of length " << i << "!" << std::endl;
            usage(argv[0]);
        }
    }
    
    if ( (argc > paramIdx) && (strcmp(argv[paramIdx], "--compact") == 0) ) {
        compactOutputFlag = true;
        ++paramIdx;
    }

    if ( (argc > paramIdx) && (strcmp(argv[paramIdx], "--sort") == 0) ) {
        sortedOutput = true;
        ++paramIdx;
    }

    //
    read_optional_params(argc, argv, paramIdx);

    std::cerr << "Starting epochal phrase table extraction with params:" << lossyCountersParams << std::endl;
    std::cerr << "Output will be " << (sortedOutput ? "sorted" : "unsorted") << "." << std::endl;

    // open input files
    std::ifstream eFile(fileNameE);
    std::ifstream fFile(fileNameF);
    std::ifstream aFile(fileNameA);

    // open output files
    if (translationFlag) {
        if (sortedOutput) {
            extractFile.open((fileNameExtract + ".sorted").c_str());
            extractFileInv.open((fileNameExtract + ".inv.sorted").c_str());
        }
        else {
            extractFile.open(fileNameExtract.c_str());
            extractFileInv.open((fileNameExtract + ".inv").c_str());
        }
    }
    if (orientationFlag) {
        extractFileOrientation.open((fileNameExtract + ".o").c_str());
    }

    //
    readInput(eFile, fFile, aFile);

    std::cerr << std::endl; // Leave the progress bar end on previous line.
    
    // close input files
    eFile.close();
    fFile.close();
    aFile.close();

    FlushingOutputProcessor processor(compactOutputFlag);
    processOutput(processor);

    // close output files
    if (translationFlag) {
        extractFile.close();
        extractFileInv.close();
    }
    if (orientationFlag) {
	extractFileOrientation.close();
    }

    printStats();

} // end of main()


void FlushingOutputProcessor::operator()(const std::string& srcPhrase, const std::string& tgtPhrase, const std::string& orientationInfo, const alignment_t& alignment, const size_t frequency, int mode) {

    size_t m = frequency;

    if ( _compactOutputFlag ) {
        // Prepend frequency.
        if (translationFlag && (mode >= 0)) extractFile << frequency << " ||| ";
        if (translationFlag && (mode <= 0)) extractFileInv << frequency << " ||| ";
        if (orientationFlag && (mode >= 0)) extractFileOrientation << frequency << " ||| ";
        m = 1; // Loop only once!
    }

    for ( size_t i = 0; i < m; ++i ) {

        // alignment
        if (translationFlag) {

            if (mode >= 0) extractFile << srcPhrase << " ||| " << tgtPhrase << " |||";
            if (mode <= 0) extractFileInv << tgtPhrase << " ||| " << srcPhrase << " |||";

            for ( alignment_t::const_iterator alignIter = alignment.begin(); alignIter != alignment.end(); ++alignIter ) {
                // Note that unsigned char isn't treated as numeric value by stream operators,
                // so casting is necessary.
                if (mode >= 0) extractFile << " " << (int) alignIter->first << "-" << (int) alignIter->second;
                if (mode <= 0) extractFileInv << " " << (int) alignIter->second << "-" << (int) alignIter->first;
            }

            if (mode >= 0) extractFile << "\n";
            if (mode <= 0) extractFileInv << "\n";
        }

        if (orientationFlag && (mode >= 0)) {
            extractFileOrientation << srcPhrase << " ||| " << tgtPhrase << " ||| " << orientationInfo << "\n";
        }

    }

}
