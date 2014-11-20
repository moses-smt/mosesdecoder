#!/usr/bin/env python2
import sys
import numpy
import optparse
#sys.path.append('/data/tools/nplm/python')

parser = optparse.OptionParser("%prog [options]")
parser.add_option("-p", "--nplm-python-path", type="string", dest="nplm_python_path")
parser.add_option("-i", "--input-model", type="string", dest="input_model")
parser.add_option("-o", "--output-model", type="string", dest="output_model")
parser.add_option("-n", "--null-token-index", type="int", dest="null_idx")
parser.add_option("-t", "--training-ngrams", type="string", dest="training_ngrams")


parser.set_defaults(
    nplm_python_path = '/mnt/gna0/rsennrich/tools/nplm/python',
    null_idx = 1
)
options,_ = parser.parse_args(sys.argv)

sys.path.append(options.nplm_python_path)
import nplm
from collections import defaultdict

def load_model(model_file):
    return nplm.NeuralLM.from_file(model_file)

def get_weights(path, length):
    d = [0]*length
    for line in open(path):
        last_context = int(line.split()[-2])
        d[last_context] += 1
    return d

if __name__ == "__main__":

    a = load_model(options.input_model)
    print 'before:'
    print a.input_embeddings[options.null_idx]
    weights = numpy.array(get_weights(options.training_ngrams, len(a.input_embeddings)))
    a.input_embeddings[options.null_idx] = numpy.average(numpy.array(a.input_embeddings), weights=weights, axis=0)
    print 'after:'
    print a.input_embeddings[options.null_idx]
    a.to_file(open(options.output_model,'w'))
