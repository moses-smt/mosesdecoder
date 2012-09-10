import os

from pypeline.helpers.helpers import eval_pipeline, \
    cons_wire, \
    cons_split_wire, \
    cons_unsplit_wire, \
    cons_dictionary_wire


# Build the pipeline components
def build_components(components, configuration):
  pipeline_components = dict()
  pipeline_configuration = dict()

  for component_id, module_name in components.items():
    print "Loading [%s] component from [%s]..." % (component_id, module_name)

    module = __import__(module_name, fromlist = ['configure', 'initialise'])
    
    # Component builds its own configuration object
    config_func = getattr(module, 'configure')
    component_config = config_func(configuration)
    pipeline_configuration.update(component_config)

    # Now build the component
    init_func = getattr(module, 'initialise')
    component = init_func(component_config)

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

  # Phew, build the required components
  components, component_config = build_components(component_modules, configuration)

  #
  # Wire up components
  # Description of wiring should be, in the future, alongside the component
  # specification in some kind of confuguration file. Components shall be
  # declared then used, i.e., bind a component instance to a unique component
  # identifier, then wire component instances together by identifier.
  #

  #
  # Tokenisation of source and target
  #
  tokenisation_component = \
      cons_split_wire() >> \
      (components['src_tokenizer'] ** components['trg_tokenizer']) >> \
      cons_unsplit_wire(lambda a, b: {'tokenised_src_filename': a['tokenised_src_filename'],
                                      'tokenised_trg_filename': b['tokenised_trg_filename']})

  #
  # A function that clips of the last '.' delimited string
  #
  def clip_last_bit(filename):
    bn = os.path.basename(filename)
    directory = os.path.dirname(filename)
    bits = bn.split(".")
    bits.pop()
    return os.path.join(directory, ".".join(bits))

  #
  # Cleanup components
  #
  def training_filename_mangler(a, s):
    return {'training_data_filename': clip_last_bit(a['train_src_filename']),
            'eval_src_filename': a['eval_src_filename'],
            'eval_trg_filename': a['eval_trg_filename']}

  cleanup_components = \
      cons_dictionary_wire({'tokenised_src_filename': 'src_filename',
                            'tokenised_trg_filename': 'trg_filename'}) >> \
      components['cleanup'] >> \
      cons_dictionary_wire({'cleaned_src_filename': 'src_filename',
                            'cleaned_trg_filename': 'trg_filename'}) >> \
      components['data_split'] >> \
      cons_wire(training_filename_mangler)

  #
  # Translation model training
  #
  model_training_component = \
      cons_split_wire() >> \
      components['model_training'].first() >> \
      cons_unsplit_wire(lambda a, b: {'moses_ini_file': a['moses_ini_file'],
                                      'eval_src_filename': b['eval_src_filename'],
                                      'eval_trg_filename': b['eval_trg_filename']})

  #
  # IRSTLM Build
  #
  irstlm_build_components = \
      cons_dictionary_wire({'tokenised_trg_filename': 'input_filename'}) >> \
      components['irstlm_build']

  #
  # Build entire pipeline
  #
  pipeline = tokenisation_component >> \
      (cons_split_wire() >> \
      ((cleanup_components >> model_training_component) ** irstlm_build_components)) >> \
      cons_unsplit_wire(lambda t, b: {'moses_ini_file': t['moses_ini_file'],
                                      'development_data_filename': clip_last_bit(t['eval_src_filename']),
                                      'trg_language_model_filename': b['compiled_lm_filename'],
                                      'trg_language_model_order': 3,
                                      'trg_language_model_type': 9}) >> \
      components['mert']

  #
  # Evaluate the pipeline
  #
  value = {'src_filename': src_filename,
           'trg_filename': trg_filename}
  print eval_pipeline(pipeline, value, component_config)


if __name__ == '__main__':
  import sys

  main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
