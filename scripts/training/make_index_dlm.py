#!/usr/bin/env python3

"""
NAME
    make_index_dlm.py -- transform extract_words_dlm.py output into phrase 
    table for WSD lookup

SYNOPSIS
    make_dlm_index.py [--leave-top-lemmas=NUM] <IN >OUT

DESCRIPTION
    Output of extract_words_dlm.py contains, for each target word, a cept in 
    source sentence, that this particular target word is aligned to. This 
    script does the following:
        
        1) Take all target surface forms for a source cept of surface forms;
        2) For each source cept, leave only significant surface forms:
        
           - First, take target lemmas, count how many times that lemma 
             occurred;
           - Leave only top-k lemmas;
           - Resulting set of target inflections will consist of all possible 
             inflections (found in input corpus) for the top-k lemmas.

    Output of script is a phrase table (|||-delimited phrase pais), that 
    contains pruned target inflections forms for all source cepts.

NOTES
    Script calls unix 'sort' and 'uniq' utilities, so they should be present on
    the machine. Script should be able to process large amounts of data without
    high memory consumption: most of the work is done by 'sort', and it uses
    external-memory algorithms.
"""

import optparse
import collections
import copy
import fileinput
import functools
import itertools
import operator
import subprocess
import sys
import tempfile

# ---------------------------------------------------------------------- Keys --

def cept_to_lookup_keys(cept):
    return [cept]

def target_to_lemma_key(target_word):
    return target_word.split("|")[1]

def prune_group(group, num_top_lemmas=None):
    """
    Leave only significant lemmas in group. 
    
    'group' is a list of [(freq, source_key, target_lemma)], where freq is 
    number of times source word was aligned to target lemma.

    Currently, uses stupid top-k pruning.
    """
    if num_top_lemmas is None:
        return group

    get_freq = lambda entry: entry[0]
    group = sorted(group, key=get_freq, reverse=True)
    return group[:20]

# --------------------------------------------------------------- Raw indices --

def parse_cept_and_target(input_line):
    return input_line.strip().split("\t")[-2:]

def make_indices(): 
    """
    Read stdin and return a pair of (SOURCE_INDEX, INFLECTIONS_INDEX).

    SOURCE_INDEX will be a file-like object (in text mode), that contains 
    following entries (one per line):

        [source_key]\t[target_lemma]

    SOURCE_INDEX will be stored in on-disk temporary file.

    INFLECTIONS_INDEX will be in-memory dict, that can be used to restore 
    inflections from target word lemmas:
    
        >>> # Contains set of target words (with frequencies), corresponding to 
        >>> # given lemma
        >>> INFLECTIONS_INDEX[target_lemma]
        {
            "target_1": 10,
            "target_2": 1,
            "target_3": 23,
        }

    """
    def make_frequency_counter():
        return collections.defaultdict(lambda: 0)

    SOURCE_INDEX = tempfile.NamedTemporaryFile(mode="w+t")
    INFLECTIONS_INDEX = collections.defaultdict(make_frequency_counter)

    for line in fileinput.input("-"):
        cept, target = parse_cept_and_target(line)

        lookup_keys = cept_to_lookup_keys(cept)
        lemma_key = target_to_lemma_key(target)

        for key in lookup_keys:
            SOURCE_INDEX.write("%s\t%s\n" % (key, lemma_key))
            INFLECTIONS_INDEX[lemma_key][target] += 1

    SOURCE_INDEX.seek(0)
    return (SOURCE_INDEX, INFLECTIONS_INDEX)

def merge_inflections(to, from_):
    RESULT = copy.copy(to)
    for (inflection, freq) in from_.items():
        RESULT[inflection] += freq
    return RESULT

# ------------------------------------------------- Index sorting and pruning --

def sort_file(file):
    sort = subprocess.Popen(
        ["sort"], 
        stdin=file, 
        stdout=subprocess.PIPE, 
        env={"LANG": "C"}
    )

    return sort.stdout

def append_frequencies(file):
    uniq = subprocess.Popen(["uniq", "-c"], stdin=file, stdout=subprocess.PIPE)
    return uniq.stdout

EMPTY_PLACEHOLDER = "__EMPTY__|__EMPTY__|---------------"

def parse_index(file):
    for line in file:
        line = line.decode("utf-8")

        freq_source, target_lemma = line.strip().split("\t")
        freq, source = freq_source.split(" ", 1)
        print(freq_source)
        freq = int(freq)
        if target_lemma == EMPTY_PLACEHOLDER:
            target_lemma = ""

        yield (freq, source, target_lemma)

def prune_and_expand(source_index, inflections_index, num_top_lemmas=None):
    get_source = lambda entry: entry[1]

    for source, group in itertools.groupby(source_index, key=get_source):
        group = prune_group(list(group), num_top_lemmas)

        lemmas = list(entry[2] for entry in group)
        inflection_sets = list(inflections_index[l] for l in lemmas)
        target_inflections = functools.reduce(merge_inflections, inflection_sets)

        yield source, target_inflections

def inflections_expander(inflections_index, num_top_lemmas):
    def RESULT(source_index):
        return prune_and_expand(source_index, inflections_index, num_top_lemmas)
    return RESULT

def print_index(source_index):
    for source, target_inflections in source_index:
        for (text, freq) in target_inflections.items():
            print("%s ||| %s ||| %i" % (source, text, freq))

# ---------------------------------------------------------------------- Main --

def main():
    parser = optparse.OptionParser()
    parser.add_option("--leave-top-lemmas", type=int)
    args, _ = parser.parse_args()
    
    source_index, inflections_index = make_indices()
    
    fns = [
        sort_file, 
        append_frequencies, 
        parse_index,
        inflections_expander(inflections_index, args.leave_top_lemmas),
        print_index
    ]

    source_index = functools.reduce(lambda x, fn: fn(x), fns, source_index)

if __name__ == "__main__":
    main()
