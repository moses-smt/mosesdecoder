#!/usr/bin/env python
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""Create a test corpus, using a previously pruned vocabulary."""


import logging
import optparse
import os
import os.path
import sys

import extract


def read_vocab(filename, offset=0):
    vocab = {}
    for i, line in enumerate(open(filename)):
        vocab[line.strip()] = i + offset
    return vocab, i + offset


def main():
    logging.basicConfig(
        format='%(asctime)s %(levelname)s: %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S', level=logging.DEBUG)
    parser = optparse.OptionParser("%prog [options]")
    parser.add_option(
        "-e", "--target-language", type="string", dest="target_language")
    parser.add_option(
        "-f", "--source-language", type="string", dest="source_language")
    parser.add_option(
        "-c", "--corpus", type="string", dest="corpus_stem")
    parser.add_option(
        "-t", "--tagged-corpus", type="string", dest="tagged_stem")
    parser.add_option(
        "-a", "--align", type="string", dest="align_file")
    parser.add_option(
        "-w", "--working-dir", type="string", dest="working_dir")

    parser.set_defaults(
        target_language="en",
        source_language="de",
        corpus_stem="test",
        align_file="test.align",
        working_dir="working")
    options, args = parser.parse_args(sys.argv)
    if not os.path.exists(options.working_dir):
        raise Exception(
            "Working directory '%s' not found" % options.working_dir)

    m, n = None, None
    for line in open(options.working_dir + "/info"):
        name, value = line[:-1].split()
        if name == "m":
            m = int(value)
        if name == "n":
            n = int(value)
    if m is None or n is None:
        raise Exception("Info file is incomplete.")

    tvocab, offset = read_vocab(options.working_dir + "/vocab.target")
    svocab, offset = read_vocab(
        options.working_dir + "/vocab.source", offset + 1)

    file_stem = os.path.basename(options.corpus_stem)
    ofh = open(options.working_dir + "/" + file_stem + ".ngrams", "w")
    extract.get_ngrams(
        options.corpus_stem,
        options.align_file,
        options.tagged_stem,
        svocab,
        tvocab,
        options.source_language,
        options.target_language,
        m,
        n,
        ofh)

    numberized_file = options.working_dir + "/" + file_stem + ".numberized"
    ngrams_file_handle = open(
        os.path.join(options.working_dir, file_stem + ".ngrams"), 'r')
    numberized_file_handle = open(numberized_file, 'w')

    # Numberize the file.
    for line in ngrams_file_handle:
        numberized_file_handle.write(extract.numberize(
            line, m, n, svocab, tvocab))

    numberized_file_handle.close()
    ngrams_file_handle.close()


if __name__ == "__main__":
    main()
