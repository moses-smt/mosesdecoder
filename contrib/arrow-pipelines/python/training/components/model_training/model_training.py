#!/usr/bin/env python

import os, shutil, subprocess

from pypeline.helpers.helpers import cons_function_component

def configure(args):
    result = {}
    result['src_lang'] = args['src_lang']
    result['trg_lang'] = args['trg_lang']
    result['moses_installation_dir'] = args['moses_installation_dir']
    result['external_bin_dir'] = args['giza_installation_dir']
    result['model_directory'] = args['translation_model_directory']
    return result

def initialise(config):

    def process(a, s):
        infilename = os.path.abspath(a['training_data_filename'])
        workdir = os.path.abspath(config['model_directory'])
        #simply call the training perl script
        #remove the workdir if it is already there
        if os.path.exists(workdir):
            shutil.rmtree(workdir)
        os.makedirs(workdir)
        
        #local vars
        train_model_perl = os.path.abspath(config['moses_installation_dir']) + os.sep + 'scripts' + os.sep + 'training' + os.sep + 'train-model.perl'
        src_lang = config['src_lang'].lower()
        trg_lang = config['trg_lang'].lower()
        external_bin = os.path.abspath(config['external_bin_dir'])
        #create a dummy lm file
        dummy_lmfile = workdir + os.sep + 'dummy.lm'
        f = open(dummy_lmfile, 'w')
        print >> f, "dummy lm file"
        f.close()
        logfile = workdir + os.sep + 'log'
        
        #the command
        cmd = '%(train_model_perl)s -root-dir %(workdir)s -corpus %(infilename)s -f %(src_lang)s -e %(trg_lang)s -alignment grow-diag-final-and -reordering msd-bidirectional-fe -lm 0:5:%(dummy_lmfile)s:0 -external-bin-dir %(external_bin)s 2> %(logfile)s'

        cmd = cmd % locals()

        pipe = subprocess.Popen(cmd, stdin = subprocess.PIPE, stdout = subprocess.PIPE, shell=True)
        pipe.wait()

        #check the moses ini
        mosesini = workdir + os.sep + 'model' + os.sep + 'moses.ini'
        if not os.path.exists(mosesini):
            raise Exception, 'Failed training model'
        
        return {'moses_ini_file':mosesini}

    return process

if __name__ == '__main__':

    def __test():
        configuration = {'src_lang':'en',
                         'trg_lang':'lt',
                         'moses_installation_dir':os.environ['MOSES_HOME'],
                         'giza_installation_dir':os.environ['GIZA_HOME'],
                         'translation_model_directory':'model-dir'}
        values = {'training_data_filename':'/Users/ianjohnson/work/MTM-2012/corpus/training/cleantrain'}
        from pypeline.helpers.helpers import run_pipeline
        box_config = configure(configuration)
        box = initialise(box_config)
        print run_pipeline(box, values, None)

    #do some test
    __test()

