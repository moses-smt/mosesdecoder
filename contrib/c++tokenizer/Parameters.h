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
    int nthreads;
    int chunksize;
    const char *cfg_path;
    bool verbose_p;
    bool detag_p;
    bool alltag_p;
    bool entities_p;
    bool escape_p;
    bool aggro_p;
    bool supersub_p;
    bool url_p;
    bool downcase_p;
    bool normalize_p;
    bool penn_p;
    bool words_p;
    bool denumber_p;
    bool narrow_latin_p;
    bool narrow_kana_p;
    bool refined_p;
    bool unescape_p;
    bool drop_bad_p;
    bool split_p;
    bool notokenization_p;
    bool para_marks_p;
    bool split_breaks_p;

	Parameters();

    Parameters(const Parameters& _);
};


#ifdef TOKENIZER_NAMESPACE
}
#endif


