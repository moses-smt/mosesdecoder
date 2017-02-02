#!/usr/bin/env python
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

from __future__ import print_function, unicode_literals

import logging
import argparse
import subprocess
import sys
import os

logging.basicConfig(
    format='%(asctime)s %(levelname)s: %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S', level=logging.DEBUG)
parser = argparse.ArgumentParser()
parser.add_argument("-w", "--working-dir", dest="working_dir")
parser.add_argument("-c", "--corpus", dest="corpus_stem")
parser.add_argument("-l", "--nplm-home", dest="nplm_home")
parser.add_argument("-e", "--epochs", dest="epochs", type=int)
parser.add_argument("-n", "--ngram-size", dest="ngram_size", type=int)
parser.add_argument("-b", "--minibatch-size", dest="minibatch_size", type=int)
parser.add_argument("-s", "--noise", dest="noise", type=int)
parser.add_argument("-d", "--hidden", dest="hidden", type=int)
parser.add_argument(
    "-i", "--input-embedding", dest="input_embedding", type=int)
parser.add_argument(
    "-o", "--output-embedding", dest="output_embedding", type=int)
parser.add_argument("-t", "--threads", dest="threads", type=int)
parser.add_argument("-m", "--output-model", dest="output_model")
parser.add_argument("-r", "--output-dir", dest="output_dir")
parser.add_argument("-f", "--config-options-file", dest="config_options_file")
parser.add_argument("-g", "--log-file", dest="log_file")
parser.add_argument("-v", "--validation-ngrams", dest="validation_file")
parser.add_argument("-a", "--activation-function", dest="activation_fn")
parser.add_argument("-z", "--learning-rate", dest="learning_rate")
parser.add_argument("--input-words-file", dest="input_words_file")
parser.add_argument("--output-words-file", dest="output_words_file")
parser.add_argument("--input_vocab_size", dest="input_vocab_size", type=int)
parser.add_argument("--output_vocab_size", dest="output_vocab_size", type=int)
parser.add_argument("--mmap", dest="mmap", action="store_true",
    help="Use memory-mapped file (for lower memory consumption).")
parser.add_argument("--extra-settings", dest="extra_settings",
  help="Extra settings to be passed to NPLM")
parser.add_argument(
    "--train-host", dest="train_host",
    help="Execute nplm training on this host, via ssh")

parser.set_defaults(
    working_dir="working",
    corpus_stem="train.10k",
    nplm_home="/home/bhaddow/tools/nplm",
    epochs=10,
    ngram_size=14,
    minibatch_size=1000,
    noise=100,
    hidden=0,
    input_embedding=150,
    output_embedding=750,
    threads=1,
    output_model="train.10k",
    output_dir=None,
    config_options_file="config",
    log_file="log",
    validation_file=None,
    activation_fn="rectifier",
    learning_rate=1,
    input_words_file=None,
    output_words_file=None,
    input_vocab_size=0,
    output_vocab_size=0
    )


def main(options):

    vocab_command = []
    if options.input_words_file is not None:
        vocab_command += ['--input_words_file', options.input_words_file]
    if options.output_words_file is not None:
        vocab_command += ['--output_words_file', options.output_words_file]
    if options.input_vocab_size:
        vocab_command += ['--input_vocab_size', str(options.input_vocab_size)]
    if options.output_vocab_size:
        vocab_command += [
            '--output_vocab_size', str(options.output_vocab_size)]

    # Set up validation command variable to use with validation set.
    validations_command = []
    if options.validation_file is not None:
        validations_command = [
            "--validation_file", (options.validation_file + ".numberized")]

    # In order to allow for different models to be trained after the same
    # preparation step, we should provide an option for multiple output
    # directories.
    # If we have not set output_dir, set it to the same thing as the working
    # dir.

    if options.output_dir is None:
        options.output_dir = options.working_dir
    else:
        # Create output dir if necessary
        if not os.path.exists(options.output_dir):
            os.makedirs(options.output_dir)

    config_file = os.path.join(
        options.output_dir,
        options.config_options_file + '-' + options.output_model)
    log_file = os.path.join(
        options.output_dir, options.log_file + '-' + options.output_model)
    log_file_write = open(log_file, 'w')
    config_file_write = open(config_file, 'w')

    config_file_write.write("Called: " + ' '.join(sys.argv) + '\n\n')

    in_file = os.path.join(
        options.working_dir,
        os.path.basename(options.corpus_stem) + ".numberized")

    mmap_command = []
    if options.mmap:
        in_file += '.mmap'
        mmap_command = ['--mmap_file', '1']

    model_prefix = os.path.join(
        options.output_dir, options.output_model + ".model.nplm")
    train_args = []
    if options.train_host:
      train_args = ["ssh", options.train_host]
    train_args += [
        options.nplm_home + "/src/trainNeuralNetwork",
        "--train_file", in_file,
        "--num_epochs", str(options.epochs),
        "--model_prefix", model_prefix,
        "--learning_rate", str(options.learning_rate),
        "--minibatch_size", str(options.minibatch_size),
        "--num_noise_samples", str(options.noise),
        "--num_hidden", str(options.hidden),
        "--input_embedding_dimension", str(options.input_embedding),
        "--output_embedding_dimension", str(options.output_embedding),
        "--num_threads", str(options.threads),
        "--activation_function", options.activation_fn,
        "--ngram_size", str(options.ngram_size),
    ] + validations_command + vocab_command + mmap_command
    if options.extra_settings: train_args += options.extra_settings.split()
    print("Train model command: ")
    print(', '.join(train_args))

    config_file_write.write("Training step:\n" + ' '.join(train_args) + '\n')
    config_file_write.close()

    log_file_write.write("Training output:\n")
    ret = subprocess.call(
        train_args, stdout=log_file_write, stderr=log_file_write)
    if ret:
        raise Exception("Training failed")

    log_file_write.close()


if __name__ == "__main__":
    options = parser.parse_args()
    main(options)
