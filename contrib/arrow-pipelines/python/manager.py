import logging
import os

from concurrent.futures import Future, ThreadPoolExecutor
from functools import partial
from pypeline.helpers.parallel_helpers import eval_pipeline, \
    cons_function_component, \
    cons_wire, \
    cons_split_wire, \
    cons_unsplit_wire, \
    cons_dictionary_wire


#
# Some logging please
#
FORMAT = '%(asctime)-15s : %(threadName)s : %(levelname)s - %(message)s'
logging.basicConfig(format = FORMAT, level = logging.DEBUG)
logger = logging.getLogger("manager")


# Build the pipeline components
def build_components(components, configuration, executor):
  pipeline_components = dict()
  pipeline_configuration = dict()

  for component_id, module_name in components.items():
    logger.info("Loading [%s] component from [%s]..." % (component_id, module_name))

    module = __import__(module_name, fromlist = ['configure', 'initialise'])
    
    # Component builds its own configuration object
    config_func = getattr(module, 'configure')
    component_config = config_func(configuration)
    pipeline_configuration.update(component_config)

    # Now build the component
    init_func = getattr(module, 'initialise')
    component_function = init_func(component_config)

    # A wrapper for the component's function that submits to the executor
    def get_component_function_wrapper(inner_function, comp_id, mod_name):
      def component_function_wrapper(a, s):
        logger.info("Running component [%s], from module [%s], with value [%s] and state [%s]..." % \
                    (comp_id, mod_name, a, s))
        return inner_function(a, s)

      return component_function_wrapper

    # Arrowize the component
    component = cons_function_component(get_component_function_wrapper(component_function, component_id, module_name))

    # And store
    pipeline_components[component_id] = component

  return pipeline_components, pipeline_configuration


# Go!
def main(src_lang, trg_lang, src_filename, trg_filename):
  # Global configuration
  # One day, this configuration shall be constructed from
  # command line options, or a properties file.
  configuration = {
    'moses_installation_dir': os.environ['MOSES_HOME'],
    'irstlm_installation_dir': os.environ['IRSTLM'],
    'giza_installation_dir': os.environ['GIZA_HOME'],
    'src_lang': src_lang,
    'src_tokenisation_dir': './tokenisation',
    'trg_lang': trg_lang,
    'trg_tokenisation_dir': './tokenisation',
    'segment_length_limit': 60,
    'irstlm_smoothing_method': 'improved-kneser-ney',
    'language_model_directory': './language-model',
    'translation_model_directory': './translation-model',
    'mert_working_directory': './mert',
    'evaluation_data_size': 100,
    'development_data_size': 100
  }

  # The modules to load
  # In the future, the components shall be specified in some kind
  # pipeline description file.
  component_modules = {
    'src_tokenizer': 'training.components.tokenizer.src_tokenizer',
    'trg_tokenizer': 'training.components.tokenizer.trg_tokenizer',
    'cleanup': 'training.components.cleanup.cleanup',
    'data_split': 'training.components.data_split.data_split',
    'irstlm_build': 'training.components.irstlm_build.irstlm_build',
    'model_training': 'training.components.model_training.model_training',
    'mert': 'training.components.mert.mert'
  }

  # The thread pool
  executor = ThreadPoolExecutor(max_workers = 3)

  # Phew, build the required components
  components, component_config = build_components(component_modules, configuration, executor)

  #
  # Wire up components
  # Description of wiring should be, in the future, alongside the component
  # specification in some kind of confuguration file. Components shall be
  # declared then used, i.e., bind a component instance to a unique component
  # identifier, then wire component instances together by identifier.
  #

  #
  # Tokenisation of source and target...
  #
  # IRSTLM Build components
  irstlm_build_component = cons_split_wire() >> \
                           (cons_wire(lambda a, s: {'input_filename':  a['tokenised_trg_filename']}) >> \
                            components['irstlm_build']).second() >> \
                           cons_unsplit_wire(lambda t, b: {'tokenised_trg_filename': t['tokenised_trg_filename'],
                                                           'trg_language_model_filename': b['compiled_lm_filename']})

  # The complete tokenisation component
  tokenisation_component = (components['src_tokenizer'] & components['trg_tokenizer']) >> \
                           irstlm_build_component.second() >> \
                           cons_unsplit_wire(lambda t, b: {'src_filename': t['tokenised_src_filename'],
                                                           'trg_filename': b['tokenised_trg_filename'],
                                                           'trg_language_model_filename': b['trg_language_model_filename']})

  #
  # Cleanup and Data Spliting...
  #

  #
  # A function that clips off the last '.' delimited string
  #
  def clip_last_bit(filename):
    bn = os.path.basename(filename)
    directory = os.path.dirname(filename)
    bits = bn.split(".")
    bits.pop()
    return os.path.join(directory, ".".join(bits))

  cleanup_datasplit_component = components['cleanup'] >> \
                                cons_wire(lambda a, s: {'src_filename': a['cleaned_src_filename'],
                                                        'trg_filename': a['cleaned_trg_filename']}) >> \
                                components['data_split'] >> \
                                cons_wire(lambda a, s: {'training_data_filename': clip_last_bit(a['train_src_filename']),
                                                        'eval_src_filename': a['eval_src_filename'],
                                                        'eval_trg_filename': a['eval_trg_filename']})

  #
  # Translation model training
  #
  translation_model_component = cons_split_wire() >> \
                                components['model_training'].first() >> \
                                cons_unsplit_wire(lambda t, b: {'moses_ini_file': t['moses_ini_file'],
                                                                'development_data_filename': b['eval_src_filename']})

  #
  # The whole pipeline
  #
  pipeline = tokenisation_component >> \
             cons_split_wire() >> \
             (cleanup_datasplit_component >> translation_model_component).first() >> \
             cons_unsplit_wire(lambda t, b: {'moses_ini_file': t['moses_ini_file'],
                                             'development_data_filename': clip_last_bit(t['development_data_filename']),
                                             'trg_language_model_filename': b['trg_language_model_filename'],
                                             'trg_language_model_order': 3,
                                             'trg_language_model_type': 9}) >> \
             components['mert']


  #
  # The input to the pipeline
  #
  value = {'src_filename': src_filename,
           'trg_filename': trg_filename}

  #
  # Evaluate the pipeline
  #
  logger.info("Evaluating pipeline with input [%s]..." % value)
  new_value = eval_pipeline(executor, pipeline, value, component_config)

  #
  # Wait for all components to finish
  #
  executor.shutdown(True)
  
  logger.info("Pipeline evaluated to %s" % new_value)


if __name__ == '__main__':
  import sys

  main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
