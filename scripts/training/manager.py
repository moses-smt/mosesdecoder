import pypeline.helpers

box_declaration = {
  'source_box0': 'training.components.cleanup',
  'source_box1': 'training.components.cleanup'
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
  'segmelt-length-limit': 60,
}

def main():
  #for label, klass in box_declaration.iteritems():
  for label, mod_name in box_declaration.iteritems():
    print(label)
    module = __import__(mod_name, fromlist=['configure', 'initialise'])
    f = getattr(module, 'configure')
    f(configuration)

main()

