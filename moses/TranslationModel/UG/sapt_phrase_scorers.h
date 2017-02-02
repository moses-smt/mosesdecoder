// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// Phrase scoring functions for suffix array-based phrase tables
// written by Ulrich Germann
#pragma once
#include "sapt_pscore_unaligned.h"    // count # of unaligned words
#include "sapt_pscore_provenance.h"   // reward for joint phrase occ. per corpus
#include "sapt_pscore_rareness.h"     // penalty for rare occurrences (global?)
#include "sapt_pscore_length_ratio.h" // model of phrase length ratio
#include "sapt_pscore_logcnt.h"       // logs of observed counts
#include "sapt_pscore_lex1.h"         // plain vanilla Moses lexical scores
#include "sapt_pscore_pfwd.h"         // fwd phrase prob
#include "sapt_pscore_pbwd.h"         // bwd phrase prob
#include "sapt_pscore_coherence.h"    // coherence feature: good/sample-size
#include "sapt_pscore_phrasecount.h"  // phrase count
#include "sapt_pscore_wordcount.h"    // word count
#include "sapt_pscore_cumulative_bias.h" // cumulative bias score
