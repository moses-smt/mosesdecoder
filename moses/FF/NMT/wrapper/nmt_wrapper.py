import numpy
import cPickle
import sys

from encdec import parse_input, RNNEncoderDecoder
from state import prototype_phrase_state


class NMTWrapper(object):
    """ NMT Wrapper """
    def __init__(self, state_path, model_path, topn_path, source_vocab_path,
                 target_vocab_path):
        self.state_path = state_path
        self.model_path = model_path
        self.topn_path = topn_path
        self.source_vocab_path = source_vocab_path
        self.target_vocab_path = target_vocab_path

    def build(self):

        self.state = prototype_phrase_state()
        with open(self.state_path) as src:
            self.state.update(cPickle.load(src))
        if 'rolling_vocab' not in self.state:
            self.state['rolling_vocab'] = 0
        if 'save_algo' not in self.state:
            self.state['save_algo'] = 0
        if 'save_gs' not in self.state:
            self.state['save_gs'] = 0
        if 'save_iter' not in self.state:
            self.state['save_iter'] = -1
        if 'var_src_len' not in self.state:
            self.state['var_src_len'] = False

        self.state['indx_word_target'] = None
        self.state['indx_word'] = None
        self.state['word_indx'] = self.source_vocab_path
        self.state['word_indx_trgt'] = self.target_vocab_path

        self.source_vocab = cPickle.load(open(self.state['word_indx']))
        self.target_vocab = cPickle.load(open(self.state['word_indx_trgt']))

        self.num_ttables = 1000
        with open(self.topn_path, 'rb') as f:
            self.topn = cPickle.load(f)
        for elt in self.topn:
            self.topn[elt] = set(self.topn[elt][:self.num_ttables])

        rng = numpy.random.RandomState(self.state['seed'])
        self.enc_dec = RNNEncoderDecoder(self.state, rng, skip_init=True)
        self.enc_dec.build()
        self.lm_model = self.enc_dec.create_lm_model()
        self.lm_model.load(self.model_path)
        self.state = self.enc_dec.state

        self._compile()

        self.original_W_0_dec_approx_embdr = self.lm_model.params[self.lm_model.name2pos['W_0_dec_approx_embdr']].get_value()
        self.original_W2_dec_deep_softmax = self.lm_model.params[self.lm_model.name2pos['W2_dec_deep_softmax']].get_value()
        self.original_b_dec_deep_softmax = self.lm_model.params[self.lm_model.name2pos['b_dec_deep_softmax']].get_value()

        # self.lm_model.params[self.lm_model.name2pos['W_0_dec_approx_embdr']].set_value(numpy.zeros((1,1), dtype=numpy.float32))
        # self.lm_model.params[self.lm_model.name2pos['W2_dec_deep_softmax']].set_value(numpy.zeros((1,1), dtype=numpy.float32))
        # self.lm_model.params[self.lm_model.name2pos['b_dec_deep_softmax']].set_value(numpy.zeros((1), dtype=numpy.float32))

    def _compile(self):
        self.comp_repr = self.enc_dec.create_representation_computer()
        self.comp_init_states = self.enc_dec.create_initializers()
        self.comp_next_probs = self.enc_dec.create_next_probs_computer()
        self.comp_next_states = self.enc_dec.create_next_states_computer()

    def get_unk(self, words):
        unks = [1 if word in self.target_vocab.keys() else 0 for word in words]
        return unks

    def get_context_vector(self, source_sentence):
        seq = parse_input(self.state, self.source_vocab, source_sentence)
        c = self.comp_repr(seq)[0]
        num_common_words = 10000
        indices = set()
        for elt in seq[:-1]:
            if elt != 1 and elt in self.topn:
                indices = indices.union(self.topn[elt])
        indices = indices.union(set(xrange(num_common_words))) # Add common words
        eos_id = self.state['null_sym_target']
        unk_id = self.state['unk_sym_target']
        indices = indices.union(set([eos_id, unk_id]))
        indices = list(indices) # Convert back to list for advanced indexing
        indices = sorted(indices)
        self.lm_model.params[self.lm_model.name2pos['W_0_dec_approx_embdr']].set_value(self.original_W_0_dec_approx_embdr[indices])
        self.lm_model.params[self.lm_model.name2pos['W2_dec_deep_softmax']].set_value(self.original_W2_dec_deep_softmax[:, indices])
        self.lm_model.params[self.lm_model.name2pos['b_dec_deep_softmax']].set_value(self.original_b_dec_deep_softmax[indices])
        positions = {}
        for pos, idx in enumerate(indices):
            positions[idx] = pos
        return (c, positions)

    def get_log_prob(self, next_word, c, last_word="", state=None):
        return self.get_log_probs([next_word], c[0], last_word, state)

    def get_log_probs(self, next_words, c, last_word="", state=None):
        c = c[0]
        cumulated_score = 0.0
        if not last_word:
            last_word = numpy.zeros(1, dtype="int64")
        else:
            last_word = [self.target_vocab.get(last_word, self.unk_id)]

        if state is None:
            states = map(lambda x: x[None, :], self.comp_init_states(c))
        else:
            states = [state]

        for next_word in next_words:
            next_indx = self.target_vocab.get(next_word, self.unk_id)

            log_probs = numpy.log(self.comp_next_probs(c, 0, last_word,
                                                       *states)[0])
            cumulated_score += log_probs[0][next_indx]

            voc_size = log_probs.shape[1]
            word_indices = numpy.array([next_indx]) % voc_size

            new_states = self.comp_next_states(c, 0, word_indices, *states)
            last_word = word_indices
            states = [new_states[0]]

        return cumulated_score, new_states[0]

    def get_vec_log_probs(self, next_words, c, last_words=[], states=[]):
        print >> sys.stderr, "GET_VEC_LOG"
        c = c[0]
        if len(last_words) == 0:
            phrase_num = 1
        else:
            phrase_num = len(last_words)

        if len(last_words) == 1 and len(last_words[0]) == 0:
            last_words = numpy.zeros(phrase_num)
        else:
            tmp = []
            for last_word in last_words:
                if last_word == "":
                    tmp.append(0)
                else:
                    tmp.append(self.target_vocab.get(last_word,
                                                     self.unk_id))
            last_words = numpy.array(tmp, dtype="int64")

        if len(states) == 0:
            states = [numpy.repeat(self.comp_init_states(c)[0][numpy.newaxis, :], phrase_num, 0)]
        else:
            states = [numpy.concatenate(states)]

        next_indxs = [self.target_vocab.get(next_word, self.unk_id)
                      for next_word in next_words]

        log_probs = numpy.log(self.comp_next_probs(c, 0, last_words.astype("int64"),
                                                   *states)[0])
        cumulated_score = [log_probs[i][next_indxs].tolist()
                           for i in range(phrase_num)]

        new_states = []
        for val in next_indxs:
            intmp = [val] * phrase_num
            new_states.append(numpy.split(self.comp_next_states(c, 0, intmp, *states)[0], phrase_num))

        return cumulated_score, new_states, self.get_unk(next_words)

    def get_next_states(self, next_words, c, states):
        c = c[0]
        states = [numpy.concatenate(states)]
        next_indxs = [self.target_vocab.get(next_word, self.unk_id)
                      for next_word in next_words]
        return numpy.split(self.comp_next_states(c, 0, next_indxs, *states)[0])

    def get_log_prob_states(self, next_words, c, last_words=[], states=[]):
        # print >> sys.stderr, "START"
        context_vector, positions = c
        # print >> sys.stderr, "PO"
        # print >> sys.stderr, self.state['null_sym_target'], self.state['unk_sym_target']

        eos_id = positions[self.state['null_sym_target']]
        unk_id = positions[self.state['unk_sym_target']]
        # print >> sys.stderr, "PO"
        if len(last_words) == 0:
            phrase_num = 1
        else:
            phrase_num = len(last_words)

        if len(last_words) >= 1 and len(last_words[0]) == 0:
            last_words = numpy.zeros(phrase_num)
        else:
            tmp = []
            for last_word in last_words:
                if last_word == "":
                    tmp.append(eos_id)
                else:
                    tmp.append(positions.get(last_word, unk_id))
            last_words = numpy.array(tmp)
            last_words = last_words.astype("int64")
        # print >> sys.stderr, "WORDS"
        if len(states) == 0:
            states = [numpy.repeat(self.comp_init_states(context_vector)[0][numpy.newaxis, :], phrase_num, 0)]
        else:
            states = [numpy.concatenate(states)]

        # print >> sys.stderr, "STATES"
        log_probs = numpy.log(self.comp_next_probs(context_vector, 0, last_words.astype("int64"),
                                                   *states)[0])
        # print >> sys.stderr, "LOGP"
        next_indxs = [positions.get(next_word, unk_id) for next_word in next_words]
        # print >> sys.stderr, "INDX"
        cumulated_score = [log_probs[i][next_indxs[i]]
                           for i in range(phrase_num)]

        # print >> sys.stderr, "pre STATES"
        new_states = numpy.split(self.comp_next_states(context_vector, 0, next_indxs, *states)[0], phrase_num)
        # print >> sys.stderr, "NEW STATES"

        return cumulated_score, new_states, self.get_unk(next_words)

