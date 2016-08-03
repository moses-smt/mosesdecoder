'''
Translates a source file using a translation model.
'''
import sys
import argparse

import numpy
import json
import cPickle as pkl

from multiprocessing import Process, Queue
from util import load_dict


def translate_model(queue, rqueue, pid, models, options, k, normalize, verbose, nbest, return_alignment, suppress_unk):

    from nmt import (build_sampler, gen_sample, load_params,
                 init_params, init_tparams)

    from theano.sandbox.rng_mrg import MRG_RandomStreams as RandomStreams
    from theano import shared
    trng = RandomStreams(1234)
    use_noise = shared(numpy.float32(0.))

    fs_init = []
    fs_next = []

    for model, option in zip(models, options):

        # allocate model parameters
        params = init_params(option)

        # load model parameters and set theano shared variables
        params = load_params(model, params)
        tparams = init_tparams(params)

        # word index
        f_init, f_next = build_sampler(tparams, option, use_noise, trng, return_alignment=return_alignment)

        fs_init.append(f_init)
        fs_next.append(f_next)

    def _translate(seq):
        # sample given an input sequence and obtain scores
        sample, score, word_probs, alignment = gen_sample(fs_init, fs_next,
                                   numpy.array(seq).T.reshape([len(seq[0]), len(seq), 1]),
                                   trng=trng, k=k, maxlen=200,
                                   stochastic=False, argmax=False, return_alignment=return_alignment, suppress_unk=suppress_unk)

        # normalize scores according to sequence lengths
        if normalize:
            lengths = numpy.array([len(s) for s in sample])
            score = score / lengths
        if nbest:
            return sample, score, word_probs, alignment
        else:
            sidx = numpy.argmin(score)
            return sample[sidx], score[sidx], word_probs[sidx], alignment[sidx]

    while True:
        req = queue.get()
        if req is None:
            break

        idx, x = req[0], req[1]
        if verbose:
            sys.stderr.write('{0} - {1}\n'.format(pid,idx))
        seq = _translate(x)

        rqueue.put((idx, seq))

    return

# prints alignment weights for a hypothesis
# dimension (target_words+1 * source_words+1)
def print_matrix(hyp, file):
  # each target word has corresponding alignment weights
  for target_word_alignment in hyp:
    # each source hidden state has a corresponding weight
    for w in target_word_alignment:
      print >>file, w,
    print >> file, ""
  print >> file, ""

import json, io
def print_matrix_json(hyp, source, target, sid, tid, file):
  source.append("</s>")
  target.append("</s>")
  links = []
  for ti, target_word_alignment in enumerate(hyp):
    for si,w in enumerate(target_word_alignment):
      links.append((target[ti], source[si], str(w), sid, tid))
  json.dump(links,file, ensure_ascii=False)


def print_matrices(mm, file):
  for hyp in mm:
    print_matrix(hyp, file)
    print >>file, "\n"


def main(models, source_file, saveto, save_alignment, k=5,
         normalize=False, n_process=5, chr_level=False, verbose=False, nbest=False, suppress_unk=False, a_json=False, print_word_probabilities=False):
    # load model model_options
    options = []
    for model in args.models:
        try:
            with open('%s.json' % model, 'rb') as f:
                options.append(json.load(f))
        except:
            with open('%s.pkl' % model, 'rb') as f:
                options.append(pkl.load(f))

        #hacks for using old models with missing options
        if not 'dropout_embedding' in options[-1]:
            options[-1]['dropout_embedding'] = 0
        if not 'dropout_hidden' in options[-1]:
            options[-1]['dropout_hidden'] = 0
        if not 'dropout_source' in options[-1]:
            options[-1]['dropout_source'] = 0
        if not 'dropout_target' in options[-1]:
            options[-1]['dropout_target'] = 0
        if not 'factors' in options[-1]:
            options[-1]['factors'] = 1
        if not 'dim_per_factor' in options[-1]:
            options[-1]['dim_per_factor'] = [options[-1]['dim_word']]

    dictionaries = options[0]['dictionaries']

    dictionaries_source = dictionaries[:-1]
    dictionary_target = dictionaries[-1]

    # load source dictionary and invert
    word_dicts = []
    word_idicts = []
    for dictionary in dictionaries_source:
        word_dict = load_dict(dictionary)
        if options[0]['n_words_src']:
            for key, idx in word_dict.items():
                if idx >= options[0]['n_words_src']:
                    del word_dict[key]
        word_idict = dict()
        for kk, vv in word_dict.iteritems():
            word_idict[vv] = kk
        word_idict[0] = '<eos>'
        word_idict[1] = 'UNK'
        word_dicts.append(word_dict)
        word_idicts.append(word_idict)

    # load target dictionary and invert
    word_dict_trg = load_dict(dictionary_target)
    word_idict_trg = dict()
    for kk, vv in word_dict_trg.iteritems():
        word_idict_trg[vv] = kk
    word_idict_trg[0] = '<eos>'
    word_idict_trg[1] = 'UNK'

    # create input and output queues for processes
    queue = Queue()
    rqueue = Queue()
    processes = [None] * n_process
    for midx in xrange(n_process):
        processes[midx] = Process(
            target=translate_model,
            args=(queue, rqueue, midx, models, options, k, normalize, verbose, nbest, save_alignment is not None, suppress_unk))
        processes[midx].start()

    # utility function
    def _seqs2words(cc):
        ww = []
        for w in cc:
            if w == 0:
                break
            ww.append(word_idict_trg[w])
        return ' '.join(ww)

    def _send_jobs(f):
        source_sentences = []
        for idx, line in enumerate(f):
            if chr_level:
                words = list(line.decode('utf-8').strip())
            else:
                words = line.strip().split()

            x = []
            for w in words:
                w = [word_dicts[i][f] if f in word_dicts[i] else 1 for (i,f) in enumerate(w.split('|'))]
                if len(w) != options[0]['factors']:
                    sys.stderr.write('Error: expected {0} factors, but input word has {1}\n'.format(options[0]['factors'], len(w)))
                    for midx in xrange(n_process):
                        processes[midx].terminate()
                    sys.exit(1)
                x.append(w)

            x += [[0]*options[0]['factors']]
            queue.put((idx, x))
            source_sentences.append(words)
        return idx+1, source_sentences

    def _finish_processes():
        for midx in xrange(n_process):
            queue.put(None)

    def _retrieve_jobs(n_samples):
        trans = [None] * n_samples
        out_idx = 0
        for idx in xrange(n_samples):
            resp = rqueue.get()
            trans[resp[0]] = resp[1]
            if verbose and numpy.mod(idx, 10) == 0:
                sys.stderr.write('Sample {0} / {1} Done\n'.format((idx+1), n_samples))
            while out_idx < n_samples and trans[out_idx] != None:
                yield trans[out_idx]
                out_idx += 1

    sys.stderr.write('Translating {0} ...\n'.format(source_file.name))
    n_samples, source_sentences = _send_jobs(source_file)
    _finish_processes()

    for i, trans in enumerate(_retrieve_jobs(n_samples)):
        if nbest:
            samples, scores, word_probs, alignment = trans
            order = numpy.argsort(scores)
            for j in order:
                saveto.write('{0} ||| {1} ||| {2}\n'.format(i, _seqs2words(samples[j]), scores[j]))
                # print alignment matrix for each hypothesis
                # header: sentence id ||| translation ||| score ||| source ||| source_token_count+eos translation_token_count+eos
                if save_alignment is not None:
                  if a_json:
                    print_matrix_json(alignment[j], source_sentences[i], _seqs2words(samples[j]).split(), i, i+j,save_alignment)
                  else:
                    save_alignment.write('{0} ||| {1} ||| {2} ||| {3} ||| {4} {5}\n'.format(
                                        i, _seqs2words(samples[j]), scores[j], ' '.join(source_sentences[i]) , len(source_sentences[i])+1, len(samples[j])))
                    print_matrix(alignment[j], save_alignment)
        else:
            samples, scores, word_probs, alignment = trans
    
            saveto.write(_seqs2words(samples) + "\n")
            if print_word_probabilities:
                for prob in word_probs:
                    saveto.write("{} ".format(prob))
                saveto.write('\n')
            if save_alignment is not None:
              if a_json:
                print_matrix_json(trans[1], source_sentences[i], _seqs2words(trans[0]).split(), i, i,save_alignment)
              else:
                save_alignment.write('{0} ||| {1} ||| {2} ||| {3} ||| {4} {5}\n'.format(
                                      i, _seqs2words(trans[0]), 0, ' '.join(source_sentences[i]) , len(source_sentences[i])+1, len(trans[0])))
                print_matrix(trans[3], save_alignment)

    sys.stderr.write('Done\n')


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-k', type=int, default=5,
                        help="Beam size (default: %(default)s))")
    parser.add_argument('-p', type=int, default=5,
                        help="Number of processes (default: %(default)s))")
    parser.add_argument('-n', action="store_true",
                        help="Normalize scores by sentence length")
    parser.add_argument('-c', action="store_true", help="Character-level")
    parser.add_argument('-v', action="store_true", help="verbose mode.")
    parser.add_argument('--models', '-m', type=str, nargs = '+', required=True)
    parser.add_argument('--input', '-i', type=argparse.FileType('r'),
                        default=sys.stdin, metavar='PATH',
                        help="Input file (default: standard input)")
    parser.add_argument('--output', '-o', type=argparse.FileType('w'),
                        default=sys.stdout, metavar='PATH',
                        help="Output file (default: standard output)")
    parser.add_argument('--output_alignment', '-a', type=argparse.FileType('w'),
                        default=None, metavar='PATH',
                        help="Output file for alignment weights (default: standard output)")
    parser.add_argument('--json_alignment', action="store_true",
                        help="Output alignment in json format")
    parser.add_argument('--n-best', action="store_true",
                        help="Write n-best list (of size k)")
    parser.add_argument('--suppress-unk', action="store_true", help="Suppress hypotheses containing UNK.")
    parser.add_argument('--print-word-probabilities', '-wp',action="store_true", help="Print probabilities of each world")

    args = parser.parse_args()

    main(args.models, args.input,
         args.output, k=args.k, normalize=args.n, n_process=args.p,
         chr_level=args.c, verbose=args.v, nbest=args.n_best, suppress_unk=args.suppress_unk, 
         print_word_probabilities = args.print_word_probabilities, save_alignment=args.output_alignment, a_json=args.json_alignment)
