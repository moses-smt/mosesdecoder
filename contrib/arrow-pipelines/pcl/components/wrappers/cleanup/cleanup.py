def get_name():
    return 'cleanup'

def get_inputs():
    return ['src_filename', 'trg_filename']

def get_outputs():
    return ['cleaned_src_filename', 'cleaned_trg_filename']

def get_configuration():
    return ['segment_length_limit']

def configure(args):
    return {'segment_length' : args['segment_length_limit']}

def initialise(config):
    def _filter(limit, ifh1, ofh1, ifh2, ofh2):
        def _short(line):
            n = 0
            for c in line:
                if c == " ":
                    n += 1
            return n < limit

        for (l1, l2) in zip(ifh1, ifh2):
            if _short(l1) and _short(l2):
                print >>ofh1, l1,
                print >>ofh2, l2,

    def _make_cleaned_filename(filename):
        bits = filename.split(".")
        bits.insert(-1, "clean")
        return ".".join(bits)

    def _filter_main(a, s):
        limit = config['segment_length']
        (ifh1, ifh2, ofh1, ofh2) = (None, None, None, None)
        try:
            input_src_filename = a['src_filename']
            input_trg_filename = a['trg_filename']

            print "Cleanup: Cleaning [%s] and [%s]..." % (input_src_filename, input_trg_filename)

            ifh1 = open(input_src_filename, "r")
            ifh2 = open(input_trg_filename, "r")

            cleaned_src_filename = _make_cleaned_filename(input_src_filename)
            cleaned_trg_filename = _make_cleaned_filename(input_trg_filename)
            ofh1 = open(cleaned_src_filename, "w")
            ofh2 = open(cleaned_trg_filename, "w")

            _filter(limit, ifh1, ofh1, ifh2, ofh2)

            return {'cleaned_src_filename': cleaned_src_filename,
                    'cleaned_trg_filename': cleaned_trg_filename}
        finally:
            def _safe_close(fh):
                if fh is not None:
                    fh.close()
                _safe_close(ifh1)
                _safe_close(ifh2)
                _safe_close(ofh1)
                _safe_close(ofh2)
    
    return _filter_main


if __name__ == '__main__':
    import os
    import tempfile
    import test.test as thelp

    from pypeline.helpers.helpers import eval_pipeline


    def _test_main():
        configuration = {'segment_length_limit': 20}

        src_filename = tempfile.mkstemp(suffix = ".src", dir = "/tmp")
        trg_filename = tempfile.mkstemp(suffix = ".trg", dir = "/tmp")

        box_eval = {
            'src_filename': src_filename[1],
            'trg_filename': trg_filename[1],
            'cleaned_src_file_expected': src_filename[1] + ".expected",
            'cleaned_trg_file_expected': trg_filename[1] + ".expected"}

        try:
            _prep_files(box_eval)
            _run_test(configuration, box_eval)
        finally:
            _cleanup_files(box_eval)


    def _run_test(configuration, box_eval):
        box_config = configure(configuration)
        box = initialise(box_config)
    
        output = eval_pipeline(box, box_eval, box_config)
        try:
            thelp.diff(box_eval['cleaned_src_file_expected'], output['cleaned_src_filename'])
            thelp.diff(box_eval['cleaned_trg_file_expected'], output['cleaned_trg_filename'])
        finally:
            os.unlink(output['cleaned_src_filename'])
            os.unlink(output['cleaned_trg_filename'])


    def _line(line_lengths):
        def _gen_line(tokens):
            return " ".join(map(lambda n: "tok" + str(n), range(tokens)))
        return map(_gen_line, line_lengths)


    def _prep_files(box_eval):
        thelp.cat(box_eval['src_filename'], _line([10, 20, 30, 40, 17, 21]))
        thelp.cat(box_eval['trg_filename'], _line([40, 30, 20, 10, 20, 21]))
        thelp.cat(box_eval['cleaned_src_file_expected'], _line([17]))
        thelp.cat(box_eval['cleaned_trg_file_expected'], _line([20]))


        def _cleanup_files(box_eval):
            try:
                for key, filename in box_eval.items():
                    os.unlink(filename)
            except:
                pass


    _test_main()
