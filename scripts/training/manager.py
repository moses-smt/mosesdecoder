import pypeline.helpers

box_declaration = {
  'source_box0': 'training.components.cleanup.cleanup',
  'source_box1': 'training.components.cleanup.cleanup'
}

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

configuration = {
  'segment-length-limit': 60,
}

def main():
  #for label, mod_name in box_declaration.iteritems():
  for label, mod_name in box_declaration.items():
    print(label)
    module = __import__(mod_name, fromlist=['configure', 'initialise'])
    f = getattr(module, 'configure')
    config = f(configuration)

    f = getattr(module, 'initialise')
    box = f(config)
    box({ 
      'tokenised_src_file': '/home/its/tok1',
      'tokenised_trg_file': '/home/its/tok2',
      'cleaned_src_file': '/tmp/o1',
      'cleaned_trg_file': '/tmp/o2',
    })


main()

