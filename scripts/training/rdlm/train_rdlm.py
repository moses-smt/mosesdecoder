#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

from __future__ import print_function, unicode_literals

import logging
import argparse
import subprocess
import sys
import os
import codecs

# ../bilingual-lm
sys.path.append(os.path.join(os.path.dirname(sys.path[0]), 'bilingual-lm'))
import train_nplm
import extract_vocab
import extract_syntactic_ngrams

logging.basicConfig(
    format='%(asctime)s %(levelname)s: %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S', level=logging.DEBUG)
parser = argparse.ArgumentParser()
parser.add_argument(
    "--working-dir", dest="working_dir", metavar="PATH")
parser.add_argument(
    "--corpus", '-text', dest="corpus_stem", metavar="PATH", help="Input file.")
parser.add_argument(
    "--nplm-home", dest="nplm_home", metavar="PATH", required=True,
    help="Location of NPLM.")
parser.add_argument(
    "--epochs", dest="epochs", type=int, metavar="INT",
    help="Number of training epochs (default: %(default)s).")
parser.add_argument(
    "--up-context-size", dest="up_context_size", type=int, metavar="INT",
    help="Size of ancestor context (default: %(default)s).")
parser.add_argument(
    "--left-context-size", dest="left_context_size", type=int, metavar="INT",
    help="Size of sibling context (left) (default: %(default)s).")
parser.add_argument(
    "--right-context-size", dest="right_context_size", type=int,
    metavar="INT",
    help="Size of sibling context (right) (default: %(default)s).")
parser.add_argument(
    "--mode", dest="mode", choices=['head', 'label'], required=True,
    help="Type of RDLM to train (both are required for decoding).")
parser.add_argument(
    "--minibatch-size", dest="minibatch_size", type=int, metavar="INT",
    help="Minibatch size (default: %(default)s).")
parser.add_argument(
    "--noise", dest="noise", type=int, metavar="INT",
    help="Number of noise samples for NCE (default: %(default)s).")
parser.add_argument(
    "--hidden", dest="hidden", type=int, metavar="INT",
    help=(
        "Size of hidden layer (0 for single hidden layer) "
        "(default: %(default)s)"))
parser.add_argument(
    "--input-embedding", dest="input_embedding", type=int, metavar="INT",
    help="Size of input embedding layer (default: %(default)s).")
parser.add_argument(
    "--output-embedding", dest="output_embedding", type=int, metavar="INT",
    help="Size of output embedding layer (default: %(default)s).")
parser.add_argument(
    "--threads", "-t", dest="threads", type=int, metavar="INT",
    help="Number of threads (default: %(default)s).")
parser.add_argument(
    "--output-model", dest="output_model", metavar="PATH",
    help="Name of output model (default: %(default)s).")
parser.add_argument(
    "--output-dir", dest="output_dir", metavar="PATH",
    help="Output directory (default: same as working-dir).")
parser.add_argument(
    "--config-options-file", dest="config_options_file", metavar="PATH")
parser.add_argument(
    "--log-file", dest="log_file", metavar="PATH",
    help="Log file to write to (default: %(default)s).")
parser.add_argument(
    "--validation-corpus", dest="validation_corpus", metavar="PATH",
    help="Validation file (default: %(default)s).")
parser.add_argument(
    "--activation-function", dest="activation_fn",
    choices=['identity', 'rectifier', 'tanh', 'hardtanh'],
    help="Activation function (default: %(default)s).")
parser.add_argument(
    "--learning-rate", dest="learning_rate", type=float, metavar="FLOAT",
    help="Learning rate (default: %(default)s).")
parser.add_argument(
    "--input-words-file", dest="input_words_file", metavar="PATH",
    help="Input vocabulary (default: %(default)s).")
parser.add_argument(
    "--output-words-file", dest="output_words_file", metavar="PATH",
    help="Output vocabulary (default: %(default)s).")
parser.add_argument(
    "--input-vocab-size", dest="input_vocab_size", type=int, metavar="INT",
    help="Input vocabulary size (default: %(default)s).")
parser.add_argument(
    "--output-vocab-size", dest="output_vocab_size", type=int, metavar="INT",
    help="Output vocabulary size (default: %(default)s).")
parser.add_argument(
    "--mmap", dest="mmap", action="store_true",
    help="Use memory-mapped file (for lower memory consumption).")
parser.add_argument(
    "--train-host", dest="train_host",
    help="Execute nplm training on this host, via ssh")
parser.add_argument("--extra-settings", dest="extra_settings",
  help="Extra settings to be passed to NPLM")


parser.set_defaults(
    working_dir="working",
    corpus_stem="train",
    nplm_home="/home/bhaddow/tools/nplm",
    epochs=2,
    up_context_size=2,
    left_context_size=3,
    right_context_size=0,
    minibatch_size=1000,
    noise=100,
    hidden=0,
    mode='head',
    input_embedding=150,
    output_embedding=750,
    threads=4,
    output_model="train",
    output_dir=None,
    config_options_file="config",
    log_file="log",
    validation_corpus=None,
    activation_fn="rectifier",
    learning_rate=1,
    input_words_file=None,
    output_words_file=None,
    input_vocab_size=500000,
    output_vocab_size=500000)


def prepare_vocabulary(options):
    vocab_prefix = os.path.join(options.working_dir, 'vocab')
    extract_vocab_options = extract_vocab.create_parser().parse_args(
        ['--input', options.corpus_stem, '--output', vocab_prefix])
    extract_vocab.main(extract_vocab_options)

    if options.input_words_file is None:
        options.input_words_file = vocab_prefix + '.input'
        orig = vocab_prefix + '.all'
        filtered_vocab = open(orig).readlines()
        if options.input_vocab_size:
            filtered_vocab = filtered_vocab[:options.input_vocab_size]
        open(options.input_words_file, 'w').writelines(filtered_vocab)

    if options.output_words_file is None:
        options.output_words_file = vocab_prefix + '.output'
        if options.mode == 'label':
            blacklist = [
                '<null',
                '<root',
                '<start_head',
                '<dummy',
                '<head_head',
                '<stop_head',
            ]
            orig = vocab_prefix + '.special'
            filtered_vocab = open(orig).readlines()
            orig = vocab_prefix + '.nonterminals'
            filtered_vocab += open(orig).readlines()
            filtered_vocab = [
                word
                for word in filtered_vocab
                if not any(word.startswith(prefix) for prefix in blacklist)]
            if options.output_vocab_size:
                filtered_vocab = filtered_vocab[:options.output_vocab_size]
        else:
            orig = vocab_prefix + '.all'
            filtered_vocab = open(orig).readlines()[:options.output_vocab_size]
        open(options.output_words_file, 'w').writelines(filtered_vocab)


def main(options):

    if options.output_dir is None:
        options.output_dir = options.working_dir
    else:
        # Create output dir if necessary
        if not os.path.exists(options.output_dir):
            os.makedirs(options.output_dir)

    options.ngram_size = (
        2 * options.up_context_size +
        2 * options.left_context_size +
        2 * options.right_context_size
        )
    if options.mode == 'head':
        options.ngram_size += 2
    elif options.mode == 'label':
        options.ngram_size += 1

    if options.input_words_file is None or options.output_words_file is None:
        sys.stderr.write(
            "Either input vocabulary or output vocabulary not specified: "
            "extracting vocabulary from training text.\n")
        prepare_vocabulary(options)

    numberized_file = os.path.basename(options.corpus_stem) + '.numberized'
    train_file = numberized_file
    if options.mmap:
        train_file += '.mmap'

    extract_options = extract_syntactic_ngrams.create_parser().parse_args([
        '--input', options.corpus_stem,
        '--output', os.path.join(options.working_dir, numberized_file),
        '--vocab', options.input_words_file,
        '--output_vocab', options.output_words_file,
        '--right_context', str(options.right_context_size),
        '--left_context', str(options.left_context_size),
        '--up_context', str(options.up_context_size),
        '--mode', options.mode
        ])
    sys.stderr.write('extracting syntactic n-grams\n')
    extract_syntactic_ngrams.main(extract_options)

    if options.validation_corpus:
        extract_options.input = open(options.validation_corpus)
        options.validation_file = os.path.join(
            options.working_dir, os.path.basename(options.validation_corpus))
        extract_options.output = open(
            options.validation_file + '.numberized', 'w')
        sys.stderr.write('extracting syntactic n-grams (validation file)\n')
        extract_syntactic_ngrams.main(extract_options)
        extract_options.output.close()
    else:
        options.validation_file = None

    if options.mmap:
        try:
            os.remove(os.path.join(options.working_dir, train_file))
        except OSError:
            pass
        mmap_cmd = [os.path.join(options.nplm_home, 'src', 'createMmap'),
                    '--input_file',
                    os.path.join(options.working_dir, numberized_file),
                    '--output_file',
                    os.path.join(options.working_dir, train_file)
                    ]
        sys.stderr.write('creating memory-mapped file\n')
        sys.stderr.write('executing: ' + ', '.join(mmap_cmd) + '\n')
        ret = subprocess.call(mmap_cmd)
        if ret:
            raise Exception("creating memory-mapped file failed")

    sys.stderr.write('training neural network\n')
    train_nplm.main(options)

    sys.stderr.write('averaging null words\n')
    ret = subprocess.call([
        os.path.join(sys.path[0], 'average_null_embedding.py'),
        options.nplm_home,
        os.path.join(
            options.output_dir,
            options.output_model + '.model.nplm.' + str(options.epochs)),
        os.path.join(
            options.working_dir,
            numberized_file),
        os.path.join(options.output_dir, options.output_model + '.model.nplm')
        ])
    if ret:
        raise Exception("averaging null words failed")


if __name__ == "__main__":
    if sys.version_info < (3, 0):
        sys.stderr = codecs.getwriter('UTF-8')(sys.stderr)
        sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)
        sys.stdin = codecs.getreader('UTF-8')(sys.stdin)

    options = parser.parse_known_args()[0]
    if parser.parse_known_args()[1]:
        sys.stderr.write('Warning: unknown arguments: {0}\n'.format(parser.parse_known_args()[1]))
    main(options)
