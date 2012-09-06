from pypeline.helpers.helpers import cons_function_component

def configure(args):
  result = {}
  result['segment-length-limit'] = args['segment-length-limit']
  return result

def initialise():
  def _filter(limit, ifh1, ofh1, ifh2, ofh2):
    def _short(line):
      n = 0
      for c in line:
        if c is " ":
          n += 1
      print(line, n)
      return n <= limit

    for (l1, l2) in zip(ifh1, ifh2):
      if _short(l1) and _short(l2):
        print(l1, file=ofh1)
        print(l2, file=ofh2)

  def _filter_main(config, value):
    limit = config['segment-length-limit']
    (ifh1, ifh2, ofh1, ofh2) = (None, None, None, None)
    try:
      ifh1 = open(value['tokenised_src_file'], "r")
      ifh2 = open(value['tokenised_trg_file'], "r")
      ofh1 = open(value['cleaned_src_file'], "w")
      ofh2 = open(value['cleaned_trg_file'], "w")

      _filter(limit, ifh1, ofh1, ifh2, ofh2)
    finally:
      def _safe_close(fh):
        if fh is not None:
          fh.close()
      _safe_close(ifh1)
      _safe_close(ifh2)
      _safe_close(ofh1)
      _safe_close(ofh2)
    
  #return _filter_main
  return cons_function_component(_filter_main)


def _test():
  def _test_main():
    configuration = {
      'segment-length-limit': 60,
    }

    box_eval = { 
      'tokenised_src_file': '/home/its/tok1',
      'tokenised_trg_file': '/home/its/tok2',
      'cleaned_src_file': '/tmp/o1',
      'cleaned_trg_file': '/tmp/o2',
    }

    _prep_files(box_eval)
    _run_test(configuration, box_eval)
    _cleanup_files(box_eval)

  def _run_test(configuration, box_eval):
    from pypeline.helpers.helpers import run_pipeline
    box_config = configure(configuration)
    box = initialise()
    
    run_pipeline(box, box_config, box_eval)


  def _cat(filename, content):
    fh = open(filename, "w")
    print(content, file=fh)
    fh.close()

  def _prep_files(box_eval):
    _cat(box_eval['tokenised_src_file'], "line1\nline2\n")
    _cat(box_eval['tokenised_trg_file'], "line1\nline2\n")
    _cat(box_eval['cleaned_src_file'], "line1\nline2\n")
    _cat(box_eval['cleaned_trg_file'], "line1\nline2\n")

  def _cleanup_files(box_eval):
    return
    import os
    try:
      for key, filename in box_eval.items():
        os.unlink(filename)
    except:
      pass

  _test_main()

if __name__ == '__main__':
  _test()

