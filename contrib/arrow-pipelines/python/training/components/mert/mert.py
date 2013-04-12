#!/usr/bin/env python

import os, shutil, subprocess

from pypeline.helpers.helpers import cons_function_component

def configure(args):
    result = {}
    result['src_lang'] = args['src_lang']
    result['trg_lang'] = args['trg_lang']
    result['moses_installation_dir'] = args['moses_installation_dir']
    result['mert_working_dir'] = args['mert_working_directory']
    return result

def initialise(config):

    def process(a, s):
        infilename = os.path.abspath(a['development_data_filename'])
        lm_file = os.path.abspath(a['trg_language_model_filename'])
        lm_order = int(a['trg_language_model_order'])
        lm_type = int(a['trg_language_model_type'])
        orig_moses_ini = os.path.abspath(a['moses_ini_file'])
        
        if not os.path.exists(orig_moses_ini):
            raise Exception, "Error: Input moses.ini does not exist"

        workdir = os.path.abspath(config['mert_working_dir'])
        #simply call the training perl script
        #remove the workdir if it is already there
        if os.path.exists(workdir):
            shutil.rmtree(workdir)
        os.makedirs(workdir)

        #local vars
        moses_install_dir = os.path.abspath(config['moses_installation_dir'])
        mert_perl = os.path.join(moses_install_dir, 'scripts', 'training', 'mert-moses.pl')
        bin_dir = os.path.join(moses_install_dir, 'bin')
        moses_bin = os.path.join(moses_install_dir, 'bin', 'moses')
        src_file = infilename + '.' + config['src_lang']
        ref_file = infilename + '.' + config['trg_lang']
        logfile = os.path.join(workdir, 'log')
        #change lm configuration in moses ini
        moses_ini = os.path.join(workdir, 'trained-moses.ini')
        cmd = r"cat %(orig_moses_ini)s | sed '/\[lmodel-file\]/,/^[[:space:]]*$/c\[lmodel-file\]\n%(lm_type)s 0 %(lm_order)s %(lm_file)s\n' > %(moses_ini)s"
        cmd = cmd % locals()
        os.system(cmd)
        
        #the command
        cmd = '%(mert_perl)s --mertdir %(bin_dir)s --working-dir %(workdir)s %(src_file)s %(ref_file)s %(moses_bin)s %(moses_ini)s 2> %(logfile)s'
        cmd = cmd % locals()

        pipe = subprocess.Popen(cmd, stdin = subprocess.PIPE, stdout = subprocess.PIPE, shell=True)
        pipe.wait()

        #check the moses ini
        new_mosesini = os.path.join(workdir, 'moses.ini')
        if not os.path.exists(new_mosesini):
            raise Exception, 'Failed MERT'
        
        return {'moses_ini_file':new_mosesini}

    return process

if __name__ == '__main__':

    def __test():
        configuration = {'src_lang':'en',
                         'trg_lang':'lt',
                         'moses_installation_dir':os.path.abspath('../../../../'),
                         'mert_working_dir':'../../../../../tuning'}
        values = {'development_data_filename':'../../../../../corpus/tune',
                  'moses_ini_file':'../../../../../model/model/moses.ini',
                  'trg_language_model_filename':'../../../../../corpus/train.lt.lm',
                  'trg_language_model_type':9,
                  'trg_language_model_order':4}
        from pypeline.helpers.helpers import run_pipeline
        box_config = configure(configuration)
        box = initialise(configuration)
        print run_pipeline(box, values, None)

    #do some test
    __test()

