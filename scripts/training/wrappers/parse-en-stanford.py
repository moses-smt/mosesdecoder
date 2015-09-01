#!/usr/bin/python
# -*- coding: utf-8 -*-
# Author: Rico Sennrich
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""
(Hacky) wrapper around Stanford CoreNLP to produce CoNLL dependency format.
Assumes tokenized and sentence-split text.

To get Moses XML format, first projectivize the trees, then use
conll2mosesxml.py.
"""

from __future__ import print_function, unicode_literals
import os
import sys
import codecs
import argparse

from subprocess import Popen, PIPE

# hack for python2/3 compatibility
from io import open
argparse.open = open


def create_parser():
    parser = argparse.ArgumentParser(
        description=(
            """Wrapper around Stanford CoreNLP to produce CoNLL dependency format.
            Assumes that text is tokenized and has one sentence per line."""))

    parser.add_argument(
        '--stanford', type=str,
        metavar='PATH', required=True,
        help='path to Stanford CoreNLP')

    parser.add_argument(
        '--java', type=str, default='java',
        metavar='PATH',
        help='path to java executable')

    parser.add_argument(
        '--input', '-i', type=argparse.FileType('r'), default=sys.stdin,
        metavar='PATH',
        help="Input text (default: standard input).")
    parser.add_argument(
        '--output', '-o', type=argparse.FileType('w'), default=sys.stdout,
        metavar='PATH',
        help="Output text (default: standard output).")

    return parser


def process_stanford(infile, javacmd, stanfordpath):

    corenlp_jar = os.path.join(stanfordpath, 'stanford-corenlp-3.5.0.jar')
    corenlp_models_jar = os.path.join(
        stanfordpath, 'stanford-corenlp-3.5.0-models.jar')
    stanford = Popen(
        [
            javacmd,
            '-cp', "%s:%s" % (corenlp_jar, corenlp_models_jar),
            'edu.stanford.nlp.pipeline.StanfordCoreNLP',
            '-annotators', 'tokenize, ssplit, pos, depparse, lemma',
            '-ssplit.eolonly', 'true',
            '-tokenize.whitespace', 'true',
            '-numThreads', '8',
            '-textFile', '-',
            'outFile', '-',
        ],
        stdin=infile, stdout=PIPE)
    return stanford.stdout


def get_sentences(instream):
    sentence = []
    expect = 0

    for line in instream:
        if expect == 0 and line.startswith('Sentence #'):
            if sentence:
                yield sentence
            sentence = []
            expect = 1

        elif line == '\n':
            expect = 0

        elif expect == 3:
            rel, remainder = line.split('(')
            head, dep = remainder.split()
            head_int = int(head.split('-')[-1][:-1])
            dep_int = int(dep.split('-')[-1][:-1])
            sentence[dep_int - 1]['head'] = head_int
            sentence[dep_int - 1]['label'] = rel

        elif expect == 2:
            linesplit = line.split('[', 1)[1].rsplit(']', 1)[0].split('] [')
            if len(linesplit) != len(sentence):
                sys.stderr.write(
                    "Warning: mismatch in number of words in sentence\n")
                sys.stderr.write(' '.join(w['word'] for w in sentence))
                for i in range(len(sentence)):
                    sentence[i]['pos'] = '-'
                    sentence[i]['lemma'] = '-'
                    sentence[i]['head'] = 0
                    sentence[i]['label'] = '-'
                expect = 0
                continue
            for i, w in enumerate(linesplit):
                sentence[i]['pos'] = w.split(' PartOfSpeech=')[-1].split()[0]
                sentence[i]['lemma'] = w.split(' Lemma=')[-1]
            expect = 3

        elif expect == 1:
            for w in line.split():
                sentence.append({'word': w})
            expect = 2

    if sentence:
        yield sentence


def write(sentence, outstream):
    for i, w in enumerate(sentence):
        outstream.write(
            '{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}\n'.format(
                i + 1, w['word'], w['lemma'], w['pos'], w['pos'], '-',
                w['head'], w['label']))


if __name__ == '__main__':
    if sys.version_info < (3, 0):
        sys.stderr = codecs.getwriter('UTF-8')(sys.stderr)
        sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)
        sys.stdin = codecs.getreader('UTF-8')(sys.stdin)

    parser = create_parser()
    options = parser.parse_args()

    stanford = process_stanford(options.input, options.java, options.stanford)
    for sentence in get_sentences(codecs.getreader('UTF-8')(stanford)):
        write(sentence, options.output)
        options.output.write('\n')
