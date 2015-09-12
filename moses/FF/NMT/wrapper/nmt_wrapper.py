import numpy
import cPickle

from encdec import parse_input, RNNEncoderDecoder
from state import prototype_state


class NMTWrapper(object):
    """ NMT Wrapper """
    def __init__(self, state_path, model_path):
        self.state_path = state_path
        self.model_path = model_path

    def build(self):

        self.state = prototype_state()
        with open(self.state_path) as src:
            self.state.update(cPickle.load(src))

        rng = numpy.random.RandomState(self.state['seed'])
        self.enc_dec = RNNEncoderDecoder(self.state, rng, skip_init=True,
                                         compute_alignment=False)
        self.enc_dec.build()
        self.lm_model = self.enc_dec.create_lm_model()
        self.lm_model.load(self.model_path)
        self.indx_word = cPickle.load(open(self.state['word_indx'], 'rb'))

        self._compile()

        self.idict_src = cPickle.load(open(self.state['indx_word'], 'r'))
        self.state = self.enc_dec.state
        self.eos_id = self.state['null_sym_target']
        self.unk_id = self.state['unk_sym_target']
        self.word2indx = {v: k for k, v in self.lm_model.word_indxs.items()}

    def _compile(self):
        self.comp_repr = self.enc_dec.create_representation_computer()
        self.comp_init_states = self.enc_dec.create_initializers()
        self.comp_next_probs = self.enc_dec.create_next_probs_computer()
        self.comp_next_states = self.enc_dec.create_next_states_computer()

    def get_context_vector(self, source_sentence):
        seq = parse_input(self.state, self.indx_word, source_sentence)
        c = self.comp_repr(seq)[0]
        return c

    def get_log_prob(self, next_word, c, last_word="", state=None):
        return self.get_log_probs([next_word], c, last_word, state)

    def get_log_probs(self, next_words, c, last_word="", state=None):
        cumulated_score = 0.0
        if not last_word:
            last_word = numpy.zeros(1, dtype="int64")
        else:
            last_word = [self.word2indx.setdefault(last_word, self.unk_id)]

        if state is None:
            states = map(lambda x: x[None, :], self.comp_init_states(c))
        else:
            states = [state]

        for next_word in next_words:
            next_indx = self.word2indx.setdefault(next_word, self.unk_id)

            log_probs = numpy.log(self.comp_next_probs(c, 0, last_word,
                                                       *states)[0])
            cumulated_score += log_probs[0][next_indx]

            voc_size = log_probs.shape[1]
            word_indices = numpy.array([next_indx]) % voc_size

            new_states = self.comp_next_states(c, 0, word_indices, *states)
            last_word = word_indices
            states = [new_states[0]]

        return cumulated_score, new_states[0]

    def get_nbest_list(self, state):
        return None
