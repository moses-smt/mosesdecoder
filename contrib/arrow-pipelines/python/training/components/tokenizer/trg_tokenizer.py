#!/usr/bin/env python

import os

from tokenizer import Tokenizer

from pypeline.helpers.helpers import cons_function_component

def configure(args):
    result = {}
    result['trg_lang'] = args['trg_lang']
    result['trg_tokenisation_dir'] = args['trg_tokenisation_dir']
    result['moses_installation_dir'] = args['moses_installation_dir']
    return result

def initialise(config):

    def process(a, s):
        infilename = a['trg_filename']
        outfilename = Tokenizer.batch_tokenise(
            config['trg_lang'], 
            config['moses_installation_dir'],
            infilename, 
            config['trg_tokenisation_dir'])
        return {'tokenised_trg_filename':outfilename}

    return process

if __name__ == '__main__':

    def __test():
        configuration = {'trg_lang':'de',
                         'trg_tokenisation_dir':'tmptoktrg',
                         'moses_installation_dir':os.path.abspath('../../../../')}
        values = {'trg_filename':'tmp.de'}
        from pypeline.helpers.helpers import run_pipeline
        box_config = configure(configuration)
        box = initialise(configuration)
        print run_pipeline(box, values, None)

    #do some test
    __test()

