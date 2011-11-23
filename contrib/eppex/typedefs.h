/**
 * Basic eppex configuration.
 *
 * $Id$
 */

#ifndef CONFIG_H
#define	CONFIG_H

#include "IndexedPhrasesPair.h"
#include "LossyCounter.h"

// Type capable of holding all words (tokens) indices:
typedef unsigned int word_index_t;
// Type capable of holding all orientation info indices:
typedef unsigned char orientation_info_index_t;
// Phrase Pair type.
typedef IndexedPhrasesPair<orientation_info_index_t, word_index_t>  indexed_phrases_pair_t;
// Lossy Counter type.
typedef LossyCounter<indexed_phrases_pair_t> PhrasePairsLossyCounter;
// Shortcut to alignment interface.
typedef indexed_phrases_pair_t::alignment_t alignment_t;

#endif	/* CONFIG_H */
