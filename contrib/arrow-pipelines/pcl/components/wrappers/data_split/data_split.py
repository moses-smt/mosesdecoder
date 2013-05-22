def get_name():
    return 'data_split'

def get_inputs():
    return ['src_filename', 'trg_filename']

def get_outputs():
    return ['devel_src_filename', 'devel_trg_filename',
            'eval_src_filename', 'eval_trg_filename',
            'train_src_filename', 'train_trg_filename']

def get_configuration():
    return ['evaluation_data_size', 'development_data_size']

def configure(args):
    result = {}
    result['evaluate_size'] = args['evaluation_data_size']
    result['development_size'] = args['development_data_size']
    return result

def initialise(config):
    def _copy(size, inp, ofh1, ofh2):
        try:
            while size != 0:
                (l1, l2) = inp.next()
                print >>ofh1, l1,
                print >>ofh2, l2,
                size -= 1
        except StopIteration:
            pass

    def _make_split_filename(filename, data_set):
        bits = filename.split(".")
        bits.insert(-1, data_set)

        new_filename = ".".join(bits)
        return new_filename

    def _splitter_main(a, s):
        (ifh1, ifh2, ofh1, ofh2) = (None, None, None, None)
        try:
            input_src_filename = a['src_filename']
            input_trg_filename = a['trg_filename']

            ifh1 = open(input_src_filename, "r")
            ifh2 = open(input_trg_filename, "r")
            inp = iter(zip(ifh1, ifh2))

            result = {}
            for (data_set, size) in [('devel', config['development_size']),
                                     ('eval', config['evaluate_size']),
                                     ('train', -1)]:
                output_src_filename = _make_split_filename(input_src_filename, data_set)
                output_trg_filename = _make_split_filename(input_trg_filename, data_set)
                ofh1 = open(output_src_filename, "w")
                ofh2 = open(output_trg_filename, "w")

                _copy(size, inp, ofh1, ofh2)
                result[data_set + '_src_filename'] = output_src_filename
                result[data_set + '_trg_filename'] = output_trg_filename

            return result
        finally:
            def _safe_close(fh):
                if fh is not None:
                    fh.close()
                _safe_close(ifh1)
                _safe_close(ifh2)
                _safe_close(ofh1)
                _safe_close(ofh2)
    
    return _splitter_main


if __name__ == '__main__':
    import os
    import tempfile
    import test.test as thelp

    from pypeline.helpers.helpers import eval_pipeline


    def _test_main():
        configuration = {'evaluation_data_size': 7,
                         'development_data_size': 13}

        src_filename = tempfile.mkstemp(suffix = ".src", dir = "/tmp")
        trg_filename = tempfile.mkstemp(suffix = ".trg", dir = "/tmp")

        box_eval = {'src_filename': src_filename[1],
                    'trg_filename': trg_filename[1],
                    'devel_src_expected': src_filename[1] + ".devel.expected",
                    'devel_trg_expected': trg_filename[1] + ".devel.expected",
                    'eval_src_expected': src_filename[1] + ".eval.expected",
                    'eval_trg_expected': trg_filename[1] + ".eval.expected",
                    'train_src_expected': src_filename[1] + ".train.expected",
                    'train_trg_expected': trg_filename[1] + ".train.expected"}

        try:
            _prep_files(box_eval)
            _run_test(configuration, box_eval)
        finally:
            _cleanup_files(box_eval)


    def _run_test(configuration, box_eval):
        box_config = configure(configuration)
        box = initialise(box_config)
    
        output = eval_pipeline(box, box_eval, box_config)
        for data_set in ['devel', 'eval', 'train']:
            for lang in ['src', 'trg']:
                filename = output[data_set + '_' + lang + '_filename']
                filename_expected = box_eval[data_set + '_' + lang + '_expected']
            thelp.diff(filename_expected, filename)


    def _line(line_lengths):
        def _gen_line(tokens):
            return " ".join(map(lambda n: "tok" + str(n), range(tokens)))
        return map(_gen_line, line_lengths)


    def _prep_files(box_eval):
        thelp.cat(box_eval['src_filename'], _line(range(50)))
        thelp.cat(box_eval['trg_filename'], _line(range(50)))
        #expected output:
        thelp.cat(box_eval['devel_src_expected'], _line(range(0,13)))
        thelp.cat(box_eval['devel_trg_expected'], _line(range(0,13)))
        thelp.cat(box_eval['eval_src_expected'], _line(range(13,20)))
        thelp.cat(box_eval['eval_trg_expected'], _line(range(13,20)))
        thelp.cat(box_eval['train_src_expected'], _line(range(20,50)))
        thelp.cat(box_eval['train_trg_expected'], _line(range(20,50)))


    def _cleanup_files(box_eval):
        try:
            for key, filename in box_eval.items():
                os.unlink(filename)
        except:
            pass


    _test_main()
