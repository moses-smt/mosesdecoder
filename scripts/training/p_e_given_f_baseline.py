#!/usr/bin/env python3

"""
NAME
    p_e_given_f_baseline.py -- predict -ln p(e|f) based on relative frequences

SYNOPSIS

    extract_words_dlm.py PHRASES < INPUT > OUTPUT

DESCRIPTION

PARAMETERS
    PHRASES
        path to the file, produced by make_index_dlm.py

    INPUT
        path to file

    OUTPUT
        path to file
"""

import argparse
import collections
import fileinput
import math
import sys

def escape_vw(line):
    if not line.startswith("p^"):
        msg = "Wrong format: p^ prefix not found!"
        raise Exception(msg)

    return line[2:].replace("///", "|").replace("___", " ").replace(";;;", ":")

def extract_from_phrase_table(line):
    fields = line.strip().split("|||")
    SOURCE_PHRASE = fields[0].lstrip().rstrip()
    TARGET_PHRASE = fields[1].lstrip().rstrip()
    COUNT = int(fields[2])
    return SOURCE_PHRASE, TARGET_PHRASE, COUNT

def extract_source_phrase_from_vw(line):
    fields = line.split(" ")
    if len(fields) != 3:
        msg = "Wrong format: expected 3 fields delimited by spaces"
        raise Exception(msg)
    '''
    if fields[1] != "|s":
        msg = "Wrong format: expected |s namespace"
        raise Exception(msg)
    '''
    SOURCE_PHRASE = fields[2]
    return SOURCE_PHRASE

def extract_target_phrase_from_vw(line):
    fields = line.split(" ")
    TARGET_PHRASE = fields[2]
    return TARGET_PHRASE

def extract_label_from_vw(line):
    fields = line.split(" ")
    LABEL = fields[0].split(":")[0]
    return LABEL


def build_dictionaries(filename):
    JOINT_COUNTS = collections.defaultdict(lambda: 0)
    SINGLE_COUNTS = collections.defaultdict(lambda: 0)
    for line_index, line in enumerate(fileinput.input(filename)):
        source_phrase, target_phrase, count = extract_from_phrase_table(line)
        SINGLE_COUNTS[target_phrase] += count
        JOINT_COUNTS[(source_phrase, target_phrase)] += count
    return JOINT_COUNTS, SINGLE_COUNTS

def process_target_phrases(joint_counts,\
                           single_counts, \
                           alpha, \
                           vocabulary_size, \
                           stream, \
                           source_phrase):
    while True:
        line = stream.readline().strip()
        if not line:
            break
        escaped_target_phrase = extract_target_phrase_from_vw(line.strip())
        target_phrase = escape_vw(escaped_target_phrase)
        label = extract_label_from_vw(line.strip())

        joint_count = joint_counts[(source_phrase, target_phrase)]
        single_count = single_counts[target_phrase]
        LOG_PROBABILITY = \
            math.log(\
                (joint_counts[(source_phrase, target_phrase)] + alpha) /\
                (single_counts[target_phrase] + vocabulary_size * alpha)
            )

        params = (label, ":", -LOG_PROBABILITY)
        print("%s%s %.3f" % params)

def process(stream, joint_counts, single_counts, alpha, vocabulary_size):
    while True:
        line = stream.readline()
        if not line:
            break

        escaped_source_phrase = extract_source_phrase_from_vw(line.strip())
        source_phrase = escape_vw(escaped_source_phrase)

        process_target_phrases(
            joint_counts,
            single_counts,
            alpha,
            vocabulary_size,
            stream,
            source_phrase
        )

        print()
        line = stream.readline()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("phrase_table")
    parser.add_argument("-alpha", type=float, default=0.1)
    parser.add_argument("-vocabulary-size", type=int, default=1000)

    args = parser.parse_args()

    phrase_table = args.phrase_table
    alpha = args.alpha
    vocabulary_size = args.vocabulary_size

    joint_counts, single_counts = build_dictionaries(phrase_table)
    process(sys.stdin, joint_counts, single_counts, alpha, vocabulary_size)

if __name__ == "__main__":
    main()
