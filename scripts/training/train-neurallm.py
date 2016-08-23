#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""Train feed-forward neural network LM with NPLM tool.

The resulting model can be used in Moses as feature function NeuralLM.
"""

from __future__ import print_function, unicode_literals

import logging
import argparse
import subprocess
import sys
import os
import codecs

# ./bilingual-lm
sys.path.append(os.path.join(sys.path[0], 'bilingual-lm'))
import train_nplm
import averageNullEmbedding


logging.basicConfig(
    format='%(asctime)s %(levelname)s: %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S', level=logging.DEBUG)
parser = argparse.ArgumentParser()
parser.add_argument(
    "--working-dir", dest="working_dir", metavar="PATH")
parser.add_argument(
    "--corpus", '-text', dest="corpus_stem", metavar="PATH",
    help="Input file.")
parser.add_argument(
    "--nplm-home", dest="nplm_home", metavar="PATH", required=True,
    help="Location of NPLM.")
parser.add_argument(
    "--epochs", dest="epochs", type=int, metavar="INT",
    help="Number of training epochs (default: %(default)s).")
parser.add_argument(
    "--order", dest="order", type=int, metavar="INT",
    help="N-gram order of language model (default: %(default)s).")
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
    "--words-file", dest="words_file", metavar="PATH",
    help="Output vocabulary file (default: %(default)s).")
parser.add_argument(
    "--vocab-size", dest="vocab_size", type=int, metavar="INT",
    help="Vocabulary size (default: %(default)s).")
parser.add_argument(
    "--mmap", dest="mmap", action="store_true",
    help="Use memory-mapped file (for lower memory consumption).")
parser.add_argument(
    "--dropout", dest="dropout", action="store",
    help="Pass dropout to nplm")
parser.add_argument(
  "--input-dropout", dest="input_dropout", action="store",
    help="Pass input dropout to nplm")
parser.add_argument(
    "--extra-settings", dest="extra_settings",
    help="Extra settings for nplm")
parser.add_argument(
    "--train-host", dest="train_host",
    help="Execute nplm training on this host, via ssh")

parser.set_defaults(
    working_dir="working",
    corpus_stem="train",
    nplm_home="/home/bhaddow/tools/nplm",
    epochs=2,
    order=5,
    minibatch_size=1000,
    noise=100,
    hidden=0,
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
    words_file='vocab',
    vocab_size=500000)


def main(options):

    options.ngram_size = options.order

    if options.output_dir is None:
        options.output_dir = options.working_dir
    # Create dirs if necessary
    if not os.path.exists(options.working_dir):
        os.makedirs(options.working_dir)
    if not os.path.exists(options.output_dir):
        os.makedirs(options.output_dir)

    numberized_file = os.path.basename(options.corpus_stem) + '.numberized'
    vocab_file =os.path.join(options.working_dir, options.words_file) 
    train_file = numberized_file
    if options.mmap:
        train_file += '.mmap'

    extraction_cmd = []
    if options.train_host:
      extraction_cmd = ["ssh", options.train_host]
    extraction_cmd += [
        os.path.join(options.nplm_home, 'src', 'prepareNeuralLM'),
        '--train_text', options.corpus_stem,
        '--ngramize', '1',
        '--ngram_size', str(options.ngram_size),
        '--vocab_size', str(options.vocab_size),
        '--write_words_file', vocab_file,
        '--train_file', os.path.join(options.working_dir, numberized_file)
        ]

    sys.stderr.write('extracting n-grams\n')
    sys.stderr.write('executing: ' + ', '.join(extraction_cmd) + '\n')
    subprocess.check_call(extraction_cmd)

    # if dropout enabled, need to check which is the <null> vocab id
    null_id = None
    if options.dropout or options.input_dropout:
      with open(vocab_file) as vfh:
        for i,line in enumerate(vfh):
          if line[:-1].decode("utf8") == "<null>":
            null_id = i
            break
      if null_id == None:
        sys.stderr.write("WARN: could not identify null token, cannot enable dropout\n")
      else:
        if not options.extra_settings:
          options.extra_settings = ""
        if options.dropout or options.input_dropout:
          options.extra_settings += " --null_index %d " % null_id
        if options.dropout:
          options.extra_settings += " --dropout %s " % options.dropout
        if options.input_dropout:
          options.extra_settings += " --input_dropout %s " % options.input_dropout


    if options.mmap:
        try:
            os.remove(os.path.join(options.working_dir, train_file))
        except OSError:
            pass
        mmap_cmd = []
        if options.train_host:
          mmap_cmd = ["ssh", options.train_host]
        mmap_cmd += [
            os.path.join(options.nplm_home, 'src', 'createMmap'),
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

    if options.validation_corpus:

        extraction_cmd = []
        if options.train_host:
          extraction_cmd = ["ssh", options.train_host]
        extraction_cmd += [
            os.path.join(options.nplm_home, 'src', 'prepareNeuralLM'),
            '--train_text', options.validation_corpus,
            '--ngramize', '1',
            '--ngram_size', str(options.ngram_size),
            '--vocab_size', str(options.vocab_size),
            '--words_file', vocab_file,
            '--train_file', os.path.join(
                options.working_dir,
                os.path.basename(options.validation_corpus) + '.numberized')
            ]

        sys.stderr.write('extracting n-grams (validation file)\n')
        sys.stderr.write('executing: ' + ', '.join(extraction_cmd) + '\n')
        ret = subprocess.call(extraction_cmd)
        if ret:
            raise Exception("preparing neural LM failed")
        options.validation_file = os.path.join(
            options.working_dir, os.path.basename(options.validation_corpus))

    else:
        options.validation_file = None

    options.input_words_file = vocab_file
    options.output_words_file = vocab_file
    options.input_vocab_size = options.vocab_size
    options.output_vocab_size = options.vocab_size

    sys.stderr.write('training neural network\n')
    train_nplm.main(options)

    sys.stderr.write('averaging null words\n')
    output_model_file = os.path.join(
              options.output_dir,
              options.output_model + '.model.nplm.best')
    if not os.path.exists(output_model_file):
      output_model_file =  os.path.join(
              options.output_dir,
              options.output_model + '.model.nplm.' + str(options.epochs))
    average_options = averageNullEmbedding.parser.parse_args([
        '-i', output_model_file ,
        '-o', os.path.join(
            options.output_dir, options.output_model + '.model.nplm'),
        '-t', os.path.join(options.working_dir, numberized_file),
        '-p', os.path.join(options.nplm_home, 'python'),
    ])
    averageNullEmbedding.main(average_options)


if __name__ == "__main__":
    if sys.version_info < (3, 0):
        sys.stderr = codecs.getwriter('UTF-8')(sys.stderr)
        sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)
        sys.stdin = codecs.getreader('UTF-8')(sys.stdin)

    options = parser.parse_known_args()[0]
    if parser.parse_known_args()[1]:
        sys.stderr.write(
            "Warning: unknown arguments: {0}\n".format(
                parser.parse_known_args()[1]))
    main(options)
