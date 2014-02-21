# -*- coding: utf-8 -*-

#
# Sample python client. Additionally to basic functionality, shows how translation model weights can be provided to multimodel phrase table type,
# and how translation model weights can be optimized on tuning set of phrase pairs.
# translate_concurrent() shows how to use multiple moses server threads.
#

import sys
import gzip
from multiprocessing import Pool

if sys.version_info < (3, 0):
    import xmlrpclib
else:
    import xmlrpc.client as xmlrpclib


def translate(input_object, server, weights=None, model_name=None):
    """translate each sentence in an input_object (list, file-like object or other object that iterates over sentences)
       server is a xmlrpclib.ServerProxy
       model_name is the name of the PhraseDictionaryMultiModel(Counts) feature function that the weights should be applied to. It is defined in the moses.ini
       weights is a list of floats (one float per model, or one float per model per feature)
    """

    for line in input_object:
        params = {}
        params['text'] = line
        if weights:
            if not model_name:
                sys.stderr.write("Error: if you define weights, you need to specify the feature to which the weights are to be applied (e.g. PhraseDictionaryMultiModel0)\n")
                sys.exit(1)
            params['model_name'] = model_name
            params['lambda'] = weights

        print server.translate(params)


def optimize(phrase_pairs, server, model_name):

    params = {}
    params['phrase_pairs'] = phrase_pairs
    params['model_name'] = model_name
    weights = server.optimize(params)
    sys.stderr.write('weight vector (set lambda in moses.ini to this value to set as default): ')
    sys.stderr.write(','.join(map(str,weights)) + '\n')
    return weights


def read_phrase_pairs(input_object):

    pairs = []
    for line in input_object:
        line = line.split(' ||| ')
        pairs.append((line[0],line[1]))
    return pairs


#same functionality as translate(), but using multiple concurrent connections to server
def translate_concurrent(input_object, url, weights=None, num_processes=8):

    pool = Pool(processes=num_processes)
    text_args = [(line, weights, url) for line in input_object]

    for translated_line in pool.imap(translate_single_line, text_args):
        print translated_line


def translate_single_line(args):

    line, weights, url = args
    server = xmlrpclib.ServerProxy(url)

    params = {}
    params['text'] = line
    if weights:
        params['lambda'] = weights

    return server.translate(params)['text']


if __name__ == '__main__':
    url = "http://localhost:8111/RPC2"
    server = xmlrpclib.ServerProxy(url)

    phrase_pairs = read_phrase_pairs(gzip.open('/path/to/moses-regression-tests/models/multimodel/extract.sorted.gz'))
    weights = optimize(phrase_pairs, server, 'PhraseDictionaryMultiModelCounts0')

    translate(sys.stdin, server, weights, 'PhraseDictionaryMultiModelCounts0')