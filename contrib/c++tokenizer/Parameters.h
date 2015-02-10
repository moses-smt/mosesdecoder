#pragma once

#include <string>
#include <vector>

#ifdef TOKENIZER_NAMESPACE
namespace TOKENIZER_NAMESPACE {
#endif

struct Parameters
{
    std::string lang_iso;
    std::vector<std::string> args;
    std::string out_path;
    const char *cfg_path;
    bool next_cfg_p;
    bool next_output_p;
    bool verbose_p;
    bool detag_p;
    bool alltag_p;
    bool escape_p;
    bool aggro_p;
    bool supersub_p;
    bool url_p;
    bool downcase_p;
    bool penn_p;
    bool words_p;

	Parameters();
};


#ifdef TOKENIZER_NAMESPACE
}
#endif


