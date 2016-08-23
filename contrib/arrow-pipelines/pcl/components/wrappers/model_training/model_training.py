import os
import shutil
import subprocess


def get_name():
    return 'model_training'

def get_inputs():
    return ['src_filename', 'trg_filename']

def get_outputs():
    return ['moses_ini_filename']

def get_configuration():
    return ['source_language', 'target_language',
            'moses_installation_dir', 'giza_installation_dir',
            'translation_model_directory', 'alignment_method',
            'reordering_method']

# Alignment = grow-diag-final-and
# Reordering = msd-bidirectional-fe
def configure(args):
    result = {}
    result['src_lang'] = args['source_language']
    result['trg_lang'] = args['target_language']
    result['moses_installation_dir'] = args['moses_installation_dir']
    result['external_bin_dir'] = args['giza_installation_dir']
    result['model_directory'] = args['translation_model_directory']
    result['alignment'] = args['alignment_method']
    result['reordering'] = args['reordering_method']
    return result

def initialise(config):
    def process(a, s):
        get_corpora_name_fn = lambda fn: ".".join(os.path.basename(fn).split('.')[:-1])
        src_filename = os.path.abspath(a['src_filename'])
        trg_filename = os.path.abspath(a['trg_filename'])
        src_corpora_name = get_corpora_name_fn(src_filename)
        trg_corpora_name = get_corpora_name_fn(trg_filename)
        if src_corpora_name != trg_corpora_name:
            raise Exception, "Mismatch of source [%s] and target [%s] filename" % (src_filename, trg_filename)

        infilename = os.path.abspath(os.path.join(os.path.dirname(src_filename), src_corpora_name))
        workdir = os.path.abspath(config['model_directory'])
        #simply call the training perl script
        #remove the workdir if it is already there
        if os.path.exists(workdir):
            shutil.rmtree(workdir)
        os.makedirs(workdir)
        
        #local vars
        train_model_perl = os.path.abspath(os.path.join(config['moses_installation_dir'],
                                                        'scripts',
                                                        'training',
                                                        'train-model.perl'))
        src_lang = config['src_lang'].lower()
        trg_lang = config['trg_lang'].lower()
        external_bin = os.path.abspath(config['external_bin_dir'])
        #create a dummy lm file
        dummy_lmfile = os.path.join(workdir, 'dummy.lm')
        f = open(dummy_lmfile, 'w')
        print >> f, "dummy lm file"
        f.close()
        logfile = os.path.join(workdir, 'log')
        
        #the command
        alignment_method = config['alignment']
        reordering_method = config['reordering']
        cmd = '%(train_model_perl)s -root-dir %(workdir)s -corpus %(infilename)s ' \
              '-f %(src_lang)s -e %(trg_lang)s -alignment %(alignment_method)s ' \
              '-reordering %(reordering_method)s -lm 0:5:%(dummy_lmfile)s:0 ' \
              '-external-bin-dir %(external_bin)s 2> %(logfile)s'
        cmd = cmd % locals()

        pipe = subprocess.Popen(cmd, stdin = subprocess.PIPE, stdout = subprocess.PIPE, shell=True)
        pipe.wait()

        # check the moses ini
        mosesini = os.path.join(workdir, 'model', 'moses.ini')
        if not os.path.exists(mosesini):
            raise Exception, 'Failed training model'

        return {'moses_ini_filename' : mosesini}

    return process


if __name__ == '__main__':
    def __test():
        configuration = {'src_lang' : 'en',
                         'trg_lang' : 'lt',
                         'moses_installation_dir' : os.environ['MOSES_HOME'],
                         'giza_installation_dir' : os.environ['GIZA_HOME'],
                         'translation_model_directory' : 'model-dir'}
        values = {'training_data_filename' : '/Users/ianjohnson/work/MTM-2012/corpus/training/cleantrain'}
        from pypeline.helpers.helpers import run_pipeline
        box_config = configure(configuration)
        box = initialise(box_config)
        print run_pipeline(box, values, None)

    #do some test
    __test()
