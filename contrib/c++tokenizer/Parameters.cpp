#include "Parameters.h"

#ifdef TOKENIZER_NAMESPACE
namespace TOKENIZER_NAMESPACE {
#endif

Parameters::Parameters()
: cfg_path(0)
, verbose_p(false)
, detag_p(false)
, alltag_p(false)
, escape_p(true)
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
{
}

#ifdef TOKENIZER_NAMESPACE
}
#endif

