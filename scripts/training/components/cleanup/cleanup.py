from pypeline.helpers.helpers import cons_function_component

def configure(args):
  result = {}
  result['segment_length'] = args['segment_length']
  return result

def initialise():
  def _filter(limit, ifh1, ofh1, ifh2, ofh2):
    def _short(line):
      n = 0
      for c in line:
        if c == " ":
          n += 1
      #print(line, ":", n)
      return n < limit

    for (l1, l2) in zip(ifh1, ifh2):
      if _short(l1) and _short(l2):
        print(l1, end='', file=ofh1)
        print(l2, end='', file=ofh2)

  def _filter_main(config, value):
    limit = config['segment_length']
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
    
  return cons_function_component(_filter_main)


if __name__ == '__main__':
  import os
  import training.components.shared.test as thelp

  def _test_main():
    configuration = {
      'segment_length': 20,
    }

    box_eval = { 
      'tokenised_src_file': '/tmp/_cleanup_test_src_input',
      'tokenised_trg_file': '/tmp/_cleanup_test_trg_input',
      'cleaned_src_file': '/tmp/_cleanup_test_src_cleaned',
      'cleaned_trg_file': '/tmp/_cleanup_test_trg_cleaned',
      'cleaned_src_file_expected': '/tmp/_cleanup_test_src_expected',
      'cleaned_trg_file_expected': '/tmp/_cleanup_test_trg_expected',
    }

    _prep_files(box_eval)
    _run_test(configuration, box_eval)
    _cleanup_files(box_eval)

  def _run_test(configuration, box_eval):
    from pypeline.helpers.helpers import run_pipeline
    box_config = configure(configuration)
    box = initialise()
    
    run_pipeline(box, box_config, box_eval)
    thelp.diff(box_eval['cleaned_src_file_expected'], box_eval['cleaned_src_file'])
    thelp.diff(box_eval['cleaned_trg_file_expected'], box_eval['cleaned_trg_file'])


  def _line(line_lengths):
    def _gen_line(tokens):
      return " ".join(map(lambda n: "tok" + str(n), range(tokens)))
    return map(_gen_line, line_lengths)

  def _prep_files(box_eval):
    thelp.cat(box_eval['tokenised_src_file'], _line([10, 20, 30, 40, 17, 21]))
    thelp.cat(box_eval['tokenised_trg_file'], _line([40, 30, 20, 10, 20, 21]))
    #expected output:
    thelp.cat(box_eval['cleaned_src_file_expected'], _line([17]))
    thelp.cat(box_eval['cleaned_trg_file_expected'], _line([20]))

  def _cleanup_files(box_eval):
    try:
      for key, filename in box_eval.items():
        os.unlink(filename)
    except:
      pass

  _test_main()

