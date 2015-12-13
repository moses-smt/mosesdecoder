#!/usr/bin/env python
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

import logging
import optparse
import subprocess
import sys


def main():
    logging.basicConfig(
        format='%(asctime)s %(levelname)s: %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S', level=logging.DEBUG)
    parser = optparse.OptionParser("%prog [options]")
    parser.add_option("-w", "--working-dir", dest="working_dir")
    parser.add_option("-c", "--corpus", dest="corpus_stem")
    parser.add_option("-r", "--train-corpus", dest="train_stem")
    parser.add_option("-l", "--nplm-home", dest="nplm_home")
    parser.add_option("-e", "--epoch", dest="epoch", type="int")
    parser.add_option("-n", "--ngram-size", dest="ngram_size", type="int")
    parser.add_option(
        "-b", "--minibatch-size", dest="minibatch_size", type="int")
    parser.add_option("-t", "--threads", dest="threads", type="int")

    parser.set_defaults(
        working_dir="working",
        corpus_stem="test",
        train_stem="train.10k",
        nplm_home="/home/bhaddow/tools/nplm",
        epoch=10,
        ngram_size=14,
        minibatch_size=1000,
        threads=8)

    options, _ = parser.parse_args(sys.argv)

    model_prefix = (
        options.working_dir + "/" + options.train_stem + ".model.nplm")
    model_file = model_prefix + "." + str(options.epoch)
    test_file = options.working_dir + "/" + options.corpus_stem + ".ngrams"
    prep_file = options.working_dir + "/" + options.corpus_stem + ".prepared"
    vocab_file = options.working_dir + "/vocab"

    # TODO: Get ngram size from info file.
    prep_args = [
        options.nplm_home + "/src/prepareNeuralLM",
        "--train_text", test_file,
        "--ngram_size", str(options.ngram_size),
        "--ngramize", "0",
        "--words_file", vocab_file,
        "--train_file", prep_file,
        ]
    ret = subprocess.call(prep_args)
    if ret:
        raise Exception("Preparation failed")

    test_args = [
        options.nplm_home + "/src/testNeuralNetwork",
        "--test_file", prep_file,
        "--model_file", model_file,
        "--minibatch_size", str(options.minibatch_size),
        "--num_threads", str(options.threads),
        ]
    ret = subprocess.call(test_args)
    if ret:
        raise Exception("Testing failed")

# $ROOT/src/prepareNeuralLM --train_text $TESTFILE1 \
#     --ngram_size $NGRAM_SIZE --ngramize 1 --vocab_size $INPUT_VOCAB_SIZE \
#     --words_file $WORKDIR/words --train_file $WORKDIR/ref.ngrams || exit 1

# $ROOT/src/testNeuralNetwork --test_file $WORKDIR/ref.ngrams \
#     --model_file $OUTFILE --minibatch_size $MINIBATCH_SIZE \
#     --num_threads $THREADS || exit 1


if __name__ == "__main__":
    main()
