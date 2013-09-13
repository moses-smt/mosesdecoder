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

def prune_group(group, num_top_lemmas=None):
    if num_top_lemmas is None:
        return group

    get_freq = lambda entry: entry[0]
    group = sorted(group, key=get_freq, reverse=True)
    return group[:num_top_lemmas]

# --------------------------------------------------------------- Raw indices --

EMPTY_PLACEHOLDER = "__EMPTY__|__EMPTY__|-------------"

def parse_cept_and_target(input_line):
    cept, target = input_line.strip().split("\t")[-2:]
    if cept == EMPTY_PLACEHOLDER:
        cept = ""
    return cept, target

def make_indices(): 
    SOURCE_INDEX = tempfile.NamedTemporaryFile(mode="w+t")

    for line in fileinput.input("-"):
        cept, target = parse_cept_and_target(line)
        SOURCE_INDEX.write("%s\t%s\n" % (cept, target))

    SOURCE_INDEX.seek(0)
    return SOURCE_INDEX

# ------------------------------------------------- Index sorting and pruning --

def sort_file(file):
    sort = subprocess.Popen(
        ["sort"], 
        stdin=file, 
        stdout=subprocess.PIPE, 
        env={"LC_ALL": "C"}
    )

    return sort.stdout

def append_frequencies(file):
    uniq = subprocess.Popen(["uniq", "-c"], stdin=file, stdout=subprocess.PIPE,
      env={"LC_ALL": "C"})
    return uniq.stdout

def parse_index(file):
    for line in file:
        line = line.decode("utf-8")

        freq_source, target = line.strip().split("\t")
        freq, source = freq_source.split(" ", 1)
        freq = int(freq)

        yield (freq, source, target)

def prune(source_index, num_top_lemmas=None):
    get_source = lambda entry: entry[1]

    for source, group in itertools.groupby(source_index, key=get_source):
        group = prune_group(list(group), num_top_lemmas)
        yield source, [(target, freq) for (freq, _, target) in group]

def pruner(num_top_lemmas):
    def RESULT(source_index):
        return prune(source_index, num_top_lemmas)
    return RESULT

def print_index(source_index):
    for source, targets in source_index:
        for (text, freq) in targets:
            print("%s ||| %s ||| %i" % (source, text, freq))

# ---------------------------------------------------------------------- Main --

def main():
    parser = optparse.OptionParser()
    parser.add_option("--leave-top-lemmas", type=int)
    args, _ = parser.parse_args()
    
    source_index = make_indices()
    
    fns = [
        sort_file, 
        append_frequencies, 
        parse_index,
        pruner(args.leave_top_lemmas),
        print_index
    ]

    source_index = functools.reduce(lambda x, fn: fn(x), fns, source_index)

if __name__ == "__main__":
    main()
