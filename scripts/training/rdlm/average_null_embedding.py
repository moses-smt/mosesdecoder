#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Rico Sennrich
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""Average embeddings of special null words for RDLM.

Usage:
    average_null_embedding.py NPLM_PATH INPUT_MODEL TRAINING_FILE OUTPUT_MODEL
"""

import sys
import os
import numpy


def load_model(model_file):
    return nplm.NeuralLM.from_file(model_file)


def get_weights(path, vocab, len_context):
    d = [[0] * vocab for i in range(len_context)]
    for line in open(path):
        for i, word in enumerate(line.split()[:-1]):
            d[i][int(word)] += 1
    return d

if __name__ == "__main__":

    nplm_path = sys.argv[1]
    model_input = sys.argv[2]
    training_instances = sys.argv[3]
    model_output = sys.argv[4]

    sys.path.append(os.path.join(nplm_path, 'python'))
    import nplm

    model = load_model(model_input)

    len_context = len(open(training_instances).readline().split()) - 1

    sys.stderr.write('reading ngrams...')
    weights = numpy.array(
        get_weights(
            training_instances, len(model.input_embeddings), len_context))
    sys.stderr.write('done\n')

    for i in range(len_context):
        index = model.word_to_index_input['<null_{0}>'.format(i)]
        model.input_embeddings[index] = numpy.average(
            numpy.array(model.input_embeddings), weights=weights[i], axis=0)
    sys.stderr.write('writing model...')
    model.to_file(open(model_output, 'w'))
    sys.stderr.write('done\n')
