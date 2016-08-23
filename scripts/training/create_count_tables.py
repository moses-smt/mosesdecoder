#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Rico Sennrich <sennrich [AT] cl.uzh.ch>
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# This script creates tables that store phrase pair frequencies rather than
# probabilities.
#
# These count tables can be used for a delayed, online computation of the
# original phrase translation features.
#
# The benefit is that models can be combined quickly, with the same results
# as if we trained a model on the concatenation of all data (excepting
# differences in word alignment).
#
# Also, each model can be given a weight, which is applied to all frequencies
# of the model for the combination.

# Note: the input phrase table must have alignment information;
#       it must be unsmoothed;
#       additionally, the phrase table type PhraseDictionaryMultiModelCounts
#         requires the lexical counts files lex.counts.e2f and lex.counts.f2e
#         (obtained by using the option --write-lexical-counts in
#         train-model.perl)
# The results may differ from training on the concatenation of all data due
# to differences in word alignment, and rounding errors.


from __future__ import unicode_literals
import sys
import os
import gzip
from tempfile import NamedTemporaryFile
from subprocess import Popen, PIPE

if len(sys.argv) < 3 or len(sys.argv) > 4:
    sys.stderr.write(
        'Usage: ' +
        sys.argv[0] + " in_file out_path [prune_count]\n"
        "This script will create the files out_path/count-table.gz and "
        "out_path/count-table-target.gz\n")
    exit()


def handle_file(filename, action, fileobj=None, mode='r'):
    """support reading either from stdin, plain file or gzipped file"""

    if action == 'open':

        if mode == 'r':
            mode = 'rb'

        if mode == 'rb' and filename != '-' and not os.path.exists(filename):
            if os.path.exists(filename + '.gz'):
                filename = filename + '.gz'
            else:
                sys.stderr.write(
                    "Error: unable to open file. " +
                    filename + " - aborting.\n")
                exit()

        if filename.endswith('.gz'):
            fileobj = gzip.open(filename, mode)

        elif filename == '-':
            fileobj = sys.stdin

        else:
            fileobj = open(filename, mode)

        return fileobj

    elif action == 'close' and filename != '-':
        fileobj.close()


def sort_and_uniq(infile, outfile):

    cmd = ['sort', infile]
    fobj = handle_file(outfile, 'open', mode='w')
    sys.stderr.write(
        "Executing: LC_ALL=C " +
        ' '.join(cmd) +
        ' | uniq | gzip -c > ' + outfile + '\n')
    p_sort = Popen(cmd, env={'LC_ALL': 'C'}, stdout=PIPE)
    p_uniq = Popen(['uniq'], stdin=p_sort.stdout, stdout=PIPE)
    p_compress = Popen(['gzip', '-c'], stdin=p_uniq.stdout, stdout=fobj)
    p_compress.wait()
    fobj.close()


def create_count_lines(fobj, countobj, countobj_target, prune=0):

    i = 0
    original_pos = 0
    source = b""
    store_lines = set()
    for line in fobj:

        if not i % 100000:
            sys.stderr.write('.')
        i += 1

        line = line.split(b' ||| ')
        current_source = line[0]
        scores = line[2].split()
        comments = line[4].split()

        fs = comments[1]
        ft = comments[0]
        try:
            fst = comments[2]
        except IndexError:
            fst = str(int(round(float(scores[0]) * float(ft)))).encode()

        line[2] = b' '.join([fst, ft, fs])

        if prune:
            if current_source != source:
                write_batch(store_lines, countobj, prune)
                source = current_source
                store_lines = set()
                original_pos = 0

            store_lines.add((float(fst), original_pos, b' ||| '.join(line)))
            original_pos += 1

        else:
            countobj.write(b' ||| '.join(line))

        # Target count file.
        # If you use string formatting to make this look nicer, you may break
        # Python 3 compatibility.
        tline = b' ||| '.join([line[1], b'X', ft]) + b' ||| |||\n'
        countobj_target.write(tline)

    if prune:
        write_batch(store_lines, countobj, prune)

    countobj.close()
    countobj_target.close()


def write_batch(store_lines, outfile, prune):
    top20 = sorted(store_lines, reverse=True)[:prune]
    # Write in original_order.
    for score, original_pos, store_line in sorted(top20, key=lambda x: x[1]):
        outfile.write(store_line)


if __name__ == '__main__':

    if len(sys.argv) == 4:
        prune = int(sys.argv[3])
    else:
        prune = 0

    fileobj = handle_file(sys.argv[1], 'open')
    out_path = sys.argv[2]

    count_table_file = gzip.open(
        os.path.join(out_path, 'count-table.gz'), 'w')
    count_table_target_file = os.path.join(out_path, 'count-table-target.gz')

    count_table_target_file_temp = NamedTemporaryFile(delete=False)
    try:
        sys.stderr.write(
            "Creating temporary file for unsorted target counts file: " +
            count_table_target_file_temp.name + '\n')

        create_count_lines(
            fileobj, count_table_file, count_table_target_file_temp, prune)
        count_table_target_file_temp.close()
        sys.stderr.write(
            "Finished writing, "
            "now re-sorting and compressing target count file.\n")

        sort_and_uniq(
            count_table_target_file_temp. name, count_table_target_file)
        os.remove(count_table_target_file_temp.name)
        sys.stderr.write('Done\n')

    except BaseException:
        os.remove(count_table_target_file_temp.name)
        raise
