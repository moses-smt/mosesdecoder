/**
 * Implementation of functionality shared between counter, eppex and
 * (not yet finished) memscoring eppex.
 *
 * (C) Moses: http://www.statmt.org/moses/
 * (C) Ceslav Przywara, UFAL MFF UK, 2011
 *
 * $Id$
 */

#include <string.h>
#include <boost/tokenizer.hpp>
#include <iostream>

#include "typedefs.h"
#include "phrase-extract.h"
#include "shared.h"


std::string get_lossy_counting_params_format(void) {
    return "\n"
           "You may specify separate Lossy Counter (LC) for each phrase length or\n"
           "use shared LC for all phrase pairs with length from given inclusive interval.\n"
           "Every LC is defined by parameter in form phrase-length:error:support, where:\n"
           "  phrase-length ... a single number (eg. 2) or interval (eg. 2-4)\n"
           "  error         ... error parameter for lossy counting\n"
           "  support       ... support parameter for lossy counting\n"
           "\n"
           "Example of LC params: 1:0:0 2-4:1e-7:4e-7 5-7:2e-8:8e-8\n"
           "   - phrase pairs of length 1 will NOT be pruned\n"
           "   - phrase pairs of length from 2 to 4 (inclusive) will be pruned altogether by LC\n"
           "     with parameters support=4e-7 and error=1e-7\n"
           "   - phrase pairs of length from 5 to 7 (inclusive) will be pruned altogether by LC\n"
           "     with parameters support=8e-8 and error=2e-8\n"
           "   - max phrase length extracted will be set to 7\n"
           "\n"
           "Note: there has to be Lossy Counter defined for every phrase pair length\n"
           "up to the maximum phrase length! Following will not work: 1:0:0 5-7:2e-8:8e-8\n"
           "\n"
           "To count phrase pairs by their length a separate program (counter) may be used.\n"
           "\n"
    ;
}

bool parse_lossy_counting_params(const std::string& param) {

    // See: http://www.boost.org/doc/libs/1_42_0/libs/tokenizer/char_separator.htm
    boost::char_separator<char> separators(",:");
    boost::tokenizer<boost::char_separator<char> > tokens(param, separators);
    boost::tokenizer<boost::char_separator<char> >::iterator iter = tokens.begin();

    std::string interval = *iter;

    if ( ++iter == tokens.end() ) {
        std::cerr << "ERROR: Failed to proccess Lossy Counting param \"" << param << "\": invalid format, missing error and support parameters specification!" << std::endl;
        return false;
    }
    PhrasePairsLossyCounter::error_t error = atof((*iter).c_str());

    if ( ++iter == tokens.end() ) {
        std::cerr << "ERROR: Failed to proccess Lossy Counting param \"" << param << "\": invalid format, missing support parameter specification!" << std::endl;
        return false;
    }
    PhrasePairsLossyCounter::support_t support = atof((*iter).c_str());

    if ( (error > 0) && !(error < support) ) {
        std::cerr << "ERROR: Failed to proccess Lossy Counting param \"" << param << "\": support parameter (" << support << ") is not greater than error (" << error << ")!" << std::endl;
        return false;
    }

    // Split interval.
    boost::char_separator<char> separator("-");
    boost::tokenizer<boost::char_separator<char> > intervalTokens(interval, separator);
    iter = intervalTokens.begin();

    int from = 0, to = 0;

    from = atoi((*iter).c_str());
    if ( ++iter == intervalTokens.end() )
        to = from;
    else
        to = atoi((*iter).c_str());

    if ( ! (from <= to) ) {
        std::cerr << "ERROR: Failed to proccess Lossy Counting param \"" << param << "\": invalid interval " << from << "-" << to << " specified!" << std::endl;
        return false;
    }

    LossyCounterInstance* lci = new LossyCounterInstance(error, support);

    if ( lossyCounters.size() <= to ) {
        lossyCounters.resize(to + 1, NULL);
    }

    for ( size_t i = from; i <= to; ++i ) {
        if ( lossyCounters[i] != NULL ) {
            std::cerr << "ERROR: Failed to proccess Lossy Counting param \"" << param << "\": Lossy Counter for phrases of length " << i << " is already defined!" << std::endl;
            return false;
        }
        lossyCounters[i] = lci;
    }

    // Set maximum phrase length accordingly:
    if ( maxPhraseLength < to )
        maxPhraseLength = to;

    return true;
}

void read_optional_params(int argc, char* argv[], int optionalParamsStart) {

    for ( int i = optionalParamsStart; i < argc; i++ ) {
        if (strcmp(argv[i],"--OnlyOutputSpanInfo") == 0) {
            std::cerr << "Error: option --OnlyOutputSpanInfo is not supported!\n";
            exit(2);
        } else if (strcmp(argv[i],"orientation") == 0 || strcmp(argv[i],"--Orientation") == 0) {
            orientationFlag = true;
        } else if (strcmp(argv[i],"--NoTTable") == 0) {
            translationFlag = false;
        } else if(strcmp(argv[i],"--model") == 0) {
            if (i+1 >= argc) {
                std::cerr << "extract: syntax error, no model's information provided to the option --model " << std::endl;
                exit(1);
            }
            char* modelParams = argv[++i];
            const char* modelName = strtok(modelParams, "-");
            const char* modelType = strtok(NULL, "-");

            if(strcmp(modelName, "wbe") == 0) {
                wordModel = true;
                if(strcmp(modelType, "msd") == 0) {
                    wordType = REO_MSD;
                }
                else if(strcmp(modelType, "mslr") == 0) {
                    wordType = REO_MSLR;
                }
                else if(strcmp(modelType, "mono") == 0 || strcmp(modelType, "monotonicity") == 0) {
                    wordType = REO_MONO;
                }
                else {
                    std::cerr << "extract: syntax error, unknown reordering model type: " << modelType << std::endl;
                    exit(1);
                }
            } else if(strcmp(modelName, "phrase") == 0) {
                phraseModel = true;
                if(strcmp(modelType, "msd") == 0) {
                    phraseType = REO_MSD;
                }
                else if(strcmp(modelType, "mslr") == 0) {
                    phraseType = REO_MSLR;
                }
                else if(strcmp(modelType, "mono") == 0 || strcmp(modelType, "monotonicity") == 0) {
                    phraseType = REO_MONO;
                }
                else {
                    std::cerr << "extract: syntax error, unknown reordering model type: " << modelType << std::endl;
                    exit(1);
                }
            } else if(strcmp(modelName, "hier") == 0) {
                hierModel = true;
                if(strcmp(modelType, "msd") == 0) {
                    hierType = REO_MSD;
                }
                else if(strcmp(modelType, "mslr") == 0) {
                    hierType = REO_MSLR;
                }
                else if(strcmp(modelType, "mono") == 0 || strcmp(modelType, "monotonicity") == 0) {
                    hierType = REO_MONO;
                }
                else {
                    std::cerr << "extract: syntax error, unknown reordering model type: " << modelType << std::endl;
                    exit(1);
                }
            } else {
                std::cerr << "extract: syntax error, unknown reordering model: " << modelName << std::endl;
                exit(1);
            }

            allModelsOutputFlag = true;

        } else {
            std::cerr << "extract: syntax error, unknown option '" << std::string(argv[i]) << "'\n";
            exit(1);
        }
    }

    // default reordering model if no model selected
    // allows for the old syntax to be used
    if(orientationFlag && !allModelsOutputFlag) {
        wordModel = true;
        wordType = REO_MSD;
    }

} // end of read_optional_params()
