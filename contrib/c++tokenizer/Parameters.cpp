#include "Parameters.h"

#ifdef TOKENIZER_NAMESPACE
namespace TOKENIZER_NAMESPACE {
#endif

Parameters::Parameters()
: nthreads(0)
, chunksize(2000)
, cfg_path(0)
, verbose_p(false)
, detag_p(false)
, alltag_p(false)
, entities_p(false)
, escape_p(false)
, aggro_p(false)
, supersub_p(false)
, url_p(true)
, downcase_p(false)
, normalize_p(false)
, penn_p(false)
, words_p(false)
, denumber_p(false)
, narrow_latin_p(false)
, narrow_kana_p(false)
, refined_p(false)
, unescape_p(false)
, drop_bad_p(false)
, split_p(false)
, notokenization_p(false)
, para_marks_p(false)
, split_breaks_p(false)
{
}

#ifdef TOKENIZER_NAMESPACE
}
#endif

