#include "Parameters.h"

#ifdef TOKENIZER_NAMESPACE
namespace TOKENIZER_NAMESPACE {
#endif

Parameters::Parameters()
:cfg_path(0),
next_cfg_p(false),
next_output_p(false),
verbose_p(false),
detag_p(false),
alltag_p(false),
escape_p(true),
aggro_p(false),
supersub_p(false),
url_p(true),
downcase_p(false),
penn_p(false),
words_p(false)
{
}

#ifdef TOKENIZER_NAMESPACE
}
#endif

