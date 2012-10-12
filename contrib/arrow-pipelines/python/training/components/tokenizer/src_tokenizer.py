#!/usr/bin/env python

import os

from tokenizer import Tokenizer

from pypeline.helpers.helpers import cons_function_component

def configure(args):
    result = {}
    result['src_lang'] = args['src_lang']
    result['src_tokenisation_dir'] = args['src_tokenisation_dir']
    result['moses_installation_dir'] = args['moses_installation_dir']
    return result

def initialise(config):

    def process(a, s):
        infilename = a['src_filename']
        outfilename = Tokenizer.batch_tokenise(
            config['src_lang'], 
            config['moses_installation_dir'], 
            infilename, 
            config['src_tokenisation_dir'])
        return {'tokenised_src_filename':outfilename}

    return process

if __name__ == '__main__':

    def __test():
        configuration = {'src_lang':'de',
                         'src_tokenisation_dir':'tmptok',
                         'moses_installation_dir':os.path.abspath('../../../../')}
        values = {'src_filename':'tmp.de'}
        from pypeline.helpers.helpers import run_pipeline
        box_config = configure(configuration)
        box = initialise(configuration)
        print run_pipeline(box, values, None)

    #do some test
    __test()

