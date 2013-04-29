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


def translate(input_object, server, weights=None):

    for line in input_object:
        params = {}
        params['text'] = line
        if weights:
            params['weight-t-multimodel'] = weights

        print server.translate(params)


def optimize(phrase_pairs, server):

    params = {}
    params['phrase_pairs'] = phrase_pairs
    weights = server.optimize(params)
    sys.stderr.write(str(weights + '\n'))
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
        params['weight-t-multimodel'] = weights

    return server.translate(params)['text']


if __name__ == '__main__':
    url = "http://localhost:8111/RPC2"
    server = xmlrpclib.ServerProxy(url)

    phrase_pairs = read_phrase_pairs(gzip.open('/path/to/moses-regression-tests/models/multimodel/extract.sorted.gz'))
    weights = optimize(phrase_pairs, server)

    translate(sys.stdin, server, weights)
