import logging
import os

from concurrent.futures import Future, ThreadPoolExecutor
from functools import partial
from pypeline.helpers.helpers import eval_pipeline, \
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
      def component_function_wrapper(future, s):
        logger.info("Component [%s], from module [%s], waiting for future..." % (comp_id, mod_name))
        value = future.result()
        logger.info("Submitting component [%s], from module [%s], to executor with value [%s]..." % \
                    (comp_id, mod_name, value))
        this_future = executor.submit(inner_function, value, s)
        this_future.add_done_callback(lambda future: logger.info("Component [%s], from module [%s], completed with value [%s]" % \
                                                                 (comp_id, mod_name, future.result())))
        return this_future

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
  # Schema conversion for IRSTLM Build component
  def pre_irstlm_build_wire(f, s):
    value = f.result()

    new_value = {'input_filename':  value['tokenised_trg_filename']}

    nf = Future()
    nf.set_result(new_value)
    return nf

  # Un-split the target tokenised file and language model
  def post_tokenisation_unsplit_wire(t, b):
    t_val = t.result()
    b_val = b.result()

    value = {'tokenised_trg_filename': t_val['tokenised_trg_filename'],
             'trg_language_model_filename': b_val['compiled_lm_filename']}

    nf = Future()
    nf.set_result(value)
    return nf

  # Target tokenisation with IRSTLM Build components
  target_tokenisation_component = components['trg_tokenizer'] >> \
                                  cons_split_wire() >> \
                                  (cons_wire(pre_irstlm_build_wire) >> components['irstlm_build']).second() >> \
                                  cons_unsplit_wire(post_tokenisation_unsplit_wire)

  # Un-split the source and target tokenisations
  def post_tokenisation_unsplit_wire(t, b):
    t_val = t.result()
    b_val = b.result()

    value = {'src_filename': t_val['tokenised_src_filename'],
             'trg_filename': b_val['tokenised_trg_filename'],
             'trg_language_model_filename': b_val['trg_language_model_filename']}

    nf = Future()
    nf.set_result(value)
    return nf

  # The complete tokenisation component
  tokenisation_component = (components['src_tokenizer'] & target_tokenisation_component) >> \
                           cons_unsplit_wire(post_tokenisation_unsplit_wire)

  #
  # Cleanup and Data Spliting...
  #
  def post_cleanup_wire(f, s):
    value = f.result()

    new_value = {'src_filename': value['cleaned_src_filename'],
                 'trg_filename': value['cleaned_trg_filename']}

    nf = Future()
    nf.set_result(new_value)
    return nf

  #
  # A function that clips of the last '.' delimited string
  #
  def clip_last_bit(filename):
    bn = os.path.basename(filename)
    directory = os.path.dirname(filename)
    bits = bn.split(".")
    bits.pop()
    return os.path.join(directory, ".".join(bits))

  def training_filename_mangler(future, s):
    a = future.result()

    value = {'training_data_filename': clip_last_bit(a['train_src_filename']),
             'eval_src_filename': a['eval_src_filename'],
             'eval_trg_filename': a['eval_trg_filename']}

    new_future = Future()
    new_future.set_result(value)

    return new_future

  cleanup_datasplit_component = components['cleanup'] >> \
                                cons_wire(post_cleanup_wire) >> \
                                components['data_split'] >> \
                                cons_wire(training_filename_mangler)

  #
  # Translation model training
  #
  def post_model_training_unsplit(t, b):
    t_val = t.result()
    b_val = b.result()

    value = {'moses_ini_file': t_val['moses_ini_file'],
             'development_data_filename': b_val['eval_src_filename']}

    nf = Future()
    nf.set_result(value)
    return nf
  
  translation_model_component = cons_split_wire() >> \
                                components['model_training'].first() >> \
                                cons_unsplit_wire(post_model_training_unsplit)

  #
  # Final unsplit function
  #
  def pre_mert_unsplit(t, b):
    t_val = t.result()
    b_val = b.result()

    value = {'moses_ini_file': t_val['moses_ini_file'],
             'development_data_filename': clip_last_bit(t_val['development_data_filename']),
             'trg_language_model_filename': b_val['trg_language_model_filename'],
             'trg_language_model_order': 3,
             'trg_language_model_type': 9}

    future = Future()
    future.set_result(value)

    return future

  #
  # The whole pipeline
  #
  pipeline = tokenisation_component >> \
             cons_split_wire() >> \
             (cleanup_datasplit_component >> translation_model_component).first() >> \
             cons_unsplit_wire(pre_mert_unsplit) >> \
             components['mert']


  #
  # The input to the pipeline
  #
  value = {'src_filename': src_filename,
           'trg_filename': trg_filename}
  futurized_value = Future()
  futurized_value.set_result(value)

  #
  # Evaluate the pipeline
  #
  logger.info("Evaluating pipeline with input [%s]..." % value)
  future_value = eval_pipeline(pipeline, futurized_value, component_config)

  #
  # Wait for all components to finish
  #
  executor.shutdown(True)
  
  logger.info("Pipeline evaluated to %s" % future_value.result())


if __name__ == '__main__':
  import sys

  main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
