import os

from pypeline.helpers.helpers import eval_pipeline, \
    cons_split_wire, \
    cons_unsplit_wire, \
    cons_dictionary_wire


def defwire(src, tgt, name=None):
  return {
    'src': src,
    'tgt': tgt,
    'name': name,
  }

wires_definition = [
  defwire('source_box0', 'target_box0', 'optional name'),
  defwire('source_box0', 'target_box1'),
  defwire('source_box1', 'target_box2'),
  defwire('source_box1', 'target_box2'),
]


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
  configuration = {
    'moses_installation_dir': os.environ['MOSES_HOME'],
    'irstlm_installation_dir': os.environ['IRSTLM'],
    'src_lang': src_lang,
    'src_tokenisation_dir': './tokenisation',
    'trg_lang': trg_lang,
    'trg_tokenisation_dir': './tokenisation',
    'segment_length_limit': 60,
    'irstlm_smoothing_method': 'improved-kneser-ney',
    'language_model_directory': './language-model'
  }

  # The modules to load
  component_modules = {
    'src_tokenizer': 'training.components.tokenizer.src_tokenizer',
    'trg_tokenizer': 'training.components.tokenizer.trg_tokenizer',
    'cleanup': 'training.components.cleanup.cleanup',
    'irstlm_build': 'training.components.irstlm_build.irstlm_build'
  }

  # Phew, build the required components
  components, component_config = build_components(component_modules, configuration)

  # Wire up components
  tokenisation_component = \
      cons_split_wire() >> \
      (components['src_tokenizer'] ** components['trg_tokenizer']) >> \
      cons_unsplit_wire(lambda a, b: {'tokenised_src_filename': a['tokenised_src_filename'],
                                      'tokenised_trg_filename': b['tokenised_trg_filename']})

  # Cleanup components
  cleanup_components = \
      cons_dictionary_wire({'tokenised_src_filename': 'src_filename',
                            'tokenised_trg_filename': 'trg_filename'}) >> \
      components['cleanup']

  # IRSTLM Build
  irstlm_build_components = \
      cons_dictionary_wire({'tokenised_trg_filename': 'input_filename'}) >> \
      components['irstlm_build']

  # Build entire pipeline
  pipeline = tokenisation_component >> (cons_split_wire() >> (cleanup_components ** irstlm_build_components))
  
  # Evaluate the pipeline
  value = {'src_filename': src_filename,
           'trg_filename': trg_filename}
  print eval_pipeline(pipeline, value, component_config)


if __name__ == '__main__':
  import sys

  main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
