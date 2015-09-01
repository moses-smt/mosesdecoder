#!/usr/bin/env python
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""
Read SENNA output (from stdin), extract the parse trees, and write them in
PTB-style bracketed format (to stdout).

The SENNA output is assumed to contain tokens in the first column, POS tags
in the second column, and PSG fragments in the final column.

It is also assumed that SENNA was run through the parse-en-senna.perl wrapper,
which:

  - Substitutes the special "SENTENCE_TOO_LONG" token for sentences that
    exceed SENNA's hardcoded limit.

  - Replaces the bracket-like tokens "-LRB-", "-RRB-", etc. with "(", ")",
    etc.
"""

import optparse
import os
import sys


def main():
    usage = "usage: %prog [options]"
    parser = optparse.OptionParser(usage=usage)
    parser.add_option("--berkeley-style", action="store_true", default=False,
                      dest="berkeley",
                      help="mimic the Berkeley Parser's output format")
    (options, args) = parser.parse_args()
    if len(args) > 0:
        parser.error("incorrect number of arguments")

    tree = ""
    line_num = 0
    for line in sys.stdin:
        line_num += 1
        # Check for a blank line (the sentence delimiter).
        if line.strip() == "":
            if not balanced(tree):
                warn("unbalanced parentheses in tree ending at line %d: "
                     "discarding tree" % line_num)
                tree = ""
            tree = beautify(tree)
            if options.berkeley:
                tree = berkelify(tree)
            print tree
            tree = ""
            continue
        tokens = line.split()
        word, pos, frag = tokens[0], tokens[1], tokens[-1]
        # Check for the special "SENTENCE_TOO_LONG" token (see
        # parse-en-senna.perl)
        if word == "SENTENCE_TOO_LONG":
            continue
        # Restore -LRB-, -RRB-, etc.
        if word == "(":
            word = "-LRB-"
        elif word == ")":
            word = "-RRB-"
        elif word == "[":
            word = "-LSB-"
        elif word == "]":
            word = "-RSB-"
        elif word == "{":
            word = "-LCB-"
        elif word == "}":
            word = "-RCB-"
        tree += frag.replace("*", "(%s %s)" % (pos, word))


def balanced(s):
    num_left = 0
    num_right = 0
    for char in s:
        if char == "(":
            num_left += 1
        elif char == ")":
            num_right += 1
    return num_left == num_right


def beautify(tree):
    s = tree.replace("(", " (")
    return s.strip()


def berkelify(tree):
    if tree == "":
        return "(())"
    assert tree[0] == "("
    pos = tree.find(" (", 1)
    assert pos != -1
    old_root = tree[1:pos]
    return tree.replace(old_root, "TOP")


def warn(msg):
    prog_name = os.path.basename(sys.argv[0])
    sys.stderr.write("%s: warning: %s\n" % (prog_name, msg))


if __name__ == "__main__":
    main()
