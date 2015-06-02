#!/usr/bin/env python
# -*- coding: utf-8 -*-

# author: Rico Sennrich
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""Add flexibility scores to a phrase table half.

You usually don't have to call this script directly; to add flexibility
scores to your model, run train-model.perl with the option
"--flexibility-score" (will only affect steps 5 and 6).

Usage:
    python flexibility_score.py extract.context(.inv).sorted \
        [--Inverse] [--Hierarchical] < phrasetable > output_file
"""

from __future__ import division
from __future__ import unicode_literals

import sys
import gzip
from collections import defaultdict


class FlexScore:

    def __init__(self, inverted, hierarchical):
        self.inverted = inverted
        self.hierarchical = hierarchical

    def store_pt(self, obj):
        """Store line in dictionary.

        If we work with inverted phrase table, swap the two phrases.
        """
        src, target = obj[0], obj[1]

        if self.inverted:
            src, target = target, src

        self.phrase_pairs[src][target] = obj

    def update_contextcounts(self, obj):
        """count the number of contexts a phrase pair occurs in"""
        src, target = obj[0], obj[1]
        self.context_counts[src][target] += 1
        if obj[-1].startswith(b'<'):
            self.context_counts_l[src][target] += 1
        elif obj[-1].startswith(b'>'):
            self.context_counts_r[src][target] += 1
        elif obj[-1].startswith(b'v'):
            self.context_counts_d[src][target] += 1
        else:
            sys.stderr.write(
                b"\nERROR in line: {0}\n".format(b' ||| '.join(obj)))
            sys.stderr.write(
                b"ERROR: expecting one of '<, >, v' as context marker "
                "in context extract file.\n")
            raise ValueError

    def traverse_incrementally(self, phrasetable, flexfile):
        """Traverse phrase table and phrase extract file (with context
            information) incrementally without storing all in memory.
        """

        increment = b''
        old_increment = 1
        stack = [''] * 2

        # which phrase to use for sorting
        sort_pt = 0
        if self.inverted:
            sort_pt = 1

        while old_increment != increment:

            old_increment = increment

            self.phrase_pairs = defaultdict(dict)
            self.context_counts = defaultdict(lambda: defaultdict(int))
            self.context_counts_l = defaultdict(lambda: defaultdict(int))
            self.context_counts_r = defaultdict(lambda: defaultdict(int))
            self.context_counts_d = defaultdict(lambda: defaultdict(int))

            if stack[0]:
                self.store_pt(stack[0])
                stack[0] = b''

            if stack[1]:
                self.update_contextcounts(stack[1])
                stack[1] = b''

            for line in phrasetable:
                line = line.rstrip().split(b' ||| ')
                if line[sort_pt] != increment:
                    increment = line[sort_pt]
                    stack[0] = line
                    break
                else:
                    self.store_pt(line)

            for line in flexfile:
                line = line.rstrip().split(b' ||| ')
                if line[0] + b' |' <= old_increment + b' |':
                    self.update_contextcounts(line)

                else:
                    stack[1] = line
                    break

            yield 1

    def main(self, phrasetable, flexfile, output_object):

        i = 0
        sys.stderr.write(
            "Incrementally loading phrase table "
            "and adding flexibility score...")
        for block in self.traverse_incrementally(phrasetable, flexfile):

            self.flexprob_l = normalize(self.context_counts_l)
            self.flexprob_r = normalize(self.context_counts_r)
            self.flexprob_d = normalize(self.context_counts_d)

            # TODO: Why this lambda?  It doesn't affect sorting, does it?
            sortkey = lambda x: x + b' |'
            for src in sorted(self.phrase_pairs, key=sortkey):
                for target in sorted(self.phrase_pairs[src], key=sortkey):

                    if i % 1000000 == 0:
                        sys.stderr.write('.')
                    i += 1

                    outline = self.write_phrase_table(src, target)
                    output_object.write(outline)
        sys.stderr.write('done\n')

    def write_phrase_table(self, src, target):

        line = self.phrase_pairs[src][target]
        flexscore_l = b"{0:.6g}".format(self.flexprob_l[src][target])
        flexscore_r = b"{0:.6g}".format(self.flexprob_r[src][target])
        line[3] += b' ' + flexscore_l + b' ' + flexscore_r

        if self.hierarchical:
            try:
                flexscore_d = b"{0:.6g}".format(self.flexprob_d[src][target])
            except KeyError:
                flexscore_d = b"1"
            line[3] += b' ' + flexscore_d

        return b' ||| '.join(line) + b'\n'


def normalize(d):

    out_dict = defaultdict(dict)

    for src in d:
        total = sum(d[src].values())

        for target in d[src]:
            out_dict[src][target] = d[src][target] / total

    return out_dict


if __name__ == '__main__':

    if len(sys.argv) < 1:
        sys.stderr.write(
            "Usage: "
            "python flexibility_score.py extract.context(.inv).sorted "
            "[--Inverse] [--Hierarchical] < phrasetable > output_file\n")
        exit()

    flexfile = sys.argv[1]
    if '--Inverse' in sys.argv:
        inverted = True
    else:
        inverted = False

    if '--Hierarchical' in sys.argv:
        hierarchical = True
    else:
        hierarchical = False

    FS = FlexScore(inverted, hierarchical)
    FS.main(sys.stdin, gzip.open(flexfile, 'r'), sys.stdout)
