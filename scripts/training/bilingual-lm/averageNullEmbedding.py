#!/usr/bin/env python2
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.
import sys
import numpy
import argparse


parser = argparse.ArgumentParser(
    description=(
        "Set input embedding of <null> token to weighted average "
        "of all input embeddings"))
parser.add_argument(
    "-p", "--nplm-python-path", type=str, dest="nplm_python_path",
    default='/mnt/gna0/rsennrich/tools/nplm/python')
parser.add_argument(
    "-i", "--input-model", type=str, dest="input_model", required=True)
parser.add_argument(
    "-o", "--output-model", type=str, dest="output_model", required=True)
parser.add_argument(
    "-n", "--null-token-index", type=int, dest="null_idx", default=-1)
parser.add_argument(
    "-t", "--training-ngrams", type=str, dest="training_ngrams",
    required=True)


def load_model(model_file):
    import nplm
    return nplm.NeuralLM.from_file(model_file)


def get_weights(path, length):
    counter = [0] * length
    for line in open(path):
        last_context = int(line.split()[-2])
        counter[last_context] += 1
    return counter


def main(options):

    sys.path.append(options.nplm_python_path)

    model = load_model(options.input_model)
    if options.null_idx == -1:
        options.null_idx = model.word_to_index_input['<null>']
    sys.stderr.write('index of <null>: {0}\n'.format(options.null_idx))
    weights = numpy.array(
        get_weights(options.training_ngrams, len(model.input_embeddings)))
    model.input_embeddings[options.null_idx] = numpy.average(
        numpy.array(model.input_embeddings), weights=weights, axis=0)
    model.to_file(open(options.output_model, 'w'))


if __name__ == "__main__":
    options = parser.parse_args()
    main(options)
