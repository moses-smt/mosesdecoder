from pypeline.helpers.helpers import cons_function_component

def configure(args):
  print('ahoj')
  cons_function_component(_filter)

  result = {}
  result['segment-length-limit'] = args['segment-length-limit']
  return result

def initialise(config):
  def _filter(inp):
 
  return cons_function_component(_filter)

def _filter(config, element):
  return element

#helpers.cons_function_component(function,
#                                input_forming_function = None,
#                                output_forming_function = None,
#                                state_mutator_function = None)

