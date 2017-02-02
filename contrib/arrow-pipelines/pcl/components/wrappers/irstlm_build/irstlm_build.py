import os
import shutil
import subprocess
import tempfile


def get_name():
    return 'irstlm_build'

def get_inputs():
    return ['input_filename']

def get_outputs():
    return ['add_start_end_filename', 'lm_filename', 'compiled_lm_filename']

def get_configuration():
    return ['irstlm_installation_dir', 'irstlm_smoothing_method', 'language_model_directory']

def configure(args):
    config = dict()
    config['irstlm_install_directory'] = args['irstlm_installation_dir']
    config['smoothing_method'] = args['irstlm_smoothing_method']
    config['lm_directory'] = args['language_model_directory']
    return config    

def initialise(config):
    def process(a, s):
        # Create the LM directory if we need to
        if os.path.exists(config['lm_directory']) is False:
            os.makedirs(config['lm_directory'])

        # The filename of the file to chew through
        start_end_input_filename = a['input_filename']
        if os.path.exists(start_end_input_filename) is False:
            raise Exception("IRSTLM Build: Input file could not be found at [%s]" % start_end_input_filename)

        # Derive the output file name for the add start-end marker processor
        filename_bits = os.path.basename(start_end_input_filename).split(".")
        filename_bits[2] = "sb";
        start_end_output_filename = os.path.join(config['lm_directory'], ".".join(filename_bits))

        # Derive the output file name of the LM build
        filename_bits[2] = "lm"
        lm_filename = os.path.join(config['lm_directory'], ".".join(filename_bits))

        # Derive the compiled LM file name
        filename_bits[2] = "arpa"
        compiled_lm_filename = os.path.join(config['lm_directory'], ".".join(filename_bits))

        # First thing to do is add start and end markers
        start_end_cmdline = [os.path.join(config['irstlm_install_directory'], "bin", "add-start-end.sh")]
        infile = open(start_end_input_filename, 'r')
        outfile = open(start_end_output_filename, 'w')
        print "IRSTLM Build: Invoking [%s]..." % " ".join(start_end_cmdline)
        return_code = subprocess.check_call(start_end_cmdline, stdin = infile, stdout = outfile)
        if return_code:
            raise Exception("IRSTLM add start and end markers failed: input file = [%s], output file = [%s], return code = [%d]" % \
                            start_end_input_filename, start_end_output_filename, return_code)

        # Next build the language model
        tmp_dir = tempfile.mkdtemp(dir = "/tmp")
        try:
            build_lm_cmdline = [os.path.join(config['irstlm_install_directory'], "bin", "build-lm.sh"),
                                "-i", start_end_output_filename,
                                "-t", tmp_dir,
                                "-p",
                                "-s", config['smoothing_method'],
                                "-o", lm_filename]
            print "IRSTLM Build: Invoking [%s]..." % " ".join(build_lm_cmdline)
            return_code = subprocess.check_call(build_lm_cmdline)
            if return_code: 
                raise Exception("IRST language model failed to build: return code = [%d]" % return_code)
        finally:
            if os.path.exists(tmp_dir):
                shutil.rmtree(tmp_dir)

        # Compile the LM
        lm_filename = lm_filename + ".gz"
        compile_lm_cmdline = [os.path.join(config['irstlm_install_directory'], "bin", "compile-lm"),
                              "--text", "yes",
                              lm_filename,
                              compiled_lm_filename]
        print "IRSTLM Build: Invoking [%s]..." % " ".join(compile_lm_cmdline)
        return_code = subprocess.check_call(compile_lm_cmdline)
        if return_code:
            raise Exception("IRST language model compilation failed: return code = [%d]" % return_code)

        output = {'add_start_end_filename': start_end_output_filename,
                  'lm_filename': lm_filename,
                  'compiled_lm_filename': compiled_lm_filename}

        print "IRSTLM Build: Output = %s" % output

        return output

    return process


if __name__ == '__main__':
    from pypeline.helpers.helpers import eval_pipeline, cons_function_component

    lm_dir = os.environ["PWD"]
    configuration = {'irstlm_root': os.environ["IRSTLM"],
                     'irstlm_smoothing_method': 'improved-kneser-ney',
                     'language_model_directory': lm_dir}
    component_config = configure(configuration)
    component = initialise(component_config)

    value = eval_pipeline(cons_function_component(component),
                          {'input_filename': '/Users/ianjohnson/Dropbox/Documents/MTM2012/tokenised_files/news-commentary-v7.fr-en.tok.en'},
                          component_config)
    target = {'add_start_end_filename': os.path.join(lm_dir, 'news-commentary-v7.fr-en.sb.en'),
              'lm_filename': os.path.join(lm_dir, 'news-commentary-v7.fr-en.lm.en.gz'),
              'compiled_lm_filename': os.path.join(lm_dir, 'news-commentary-v7.fr-en.arpa.en')}
    print "Target: %s" % target
    if value != target:
        raise Exception("Massive fail!")
