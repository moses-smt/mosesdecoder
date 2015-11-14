import numpy
import logging
import pprint

import theano
import theano.tensor as TT
from theano.ifelse import ifelse
from theano.sandbox.rng_mrg import MRG_RandomStreams as RandomStreams

from groundhog.layers import\
        Layer,\
        MultiLayer,\
        SoftmaxLayer,\
        RecurrentLayer,\
        RecursiveConvolutionalLayer,\
        UnaryOp,\
        Shift,\
        LastState,\
        DropOp,\
        Concatenate
from groundhog.models import LM_Model
from groundhog.utils import sample_zeros, sample_weights_orth, init_bias, sample_weights_classic
from groundhog.utils import name2pos
import groundhog.utils as utils

logger = logging.getLogger(__name__)

class RecurrentLayerWithSearch(Layer):
    """A copy of RecurrentLayer from groundhog"""

    def __init__(self, rng,
                 n_hids,
                 c_dim=None,
                 scale=.01,
                 activation=TT.tanh,
                 bias_fn='init_bias',
                 bias_scale=0.,
                 init_fn='sample_weights',
                 gating=False,
                 reseting=False,
                 dropout=1.,
                 gater_activation=TT.nnet.sigmoid,
                 reseter_activation=TT.nnet.sigmoid,
                 weight_noise=False,
                 name=None):
        logger.debug("RecurrentLayerWithSearch is used")

        self.grad_scale = 1
        assert gating == True
        assert reseting == True
        assert dropout == 1.
        assert weight_noise == False
        updater_activation = gater_activation

        if type(init_fn) is str or type(init_fn) is unicode:
            init_fn = eval(init_fn)
        if type(bias_fn) is str or type(bias_fn) is unicode:
            bias_fn = eval(bias_fn)
        if type(activation) is str or type(activation) is unicode:
            activation = eval(activation)
        if type(updater_activation) is str or type(updater_activation) is unicode:
            updater_activation = eval(updater_activation)
        if type(reseter_activation) is str or type(reseter_activation) is unicode:
            reseter_activation = eval(reseter_activation)

        self.scale = scale
        self.activation = activation
        self.n_hids = n_hids
        self.bias_scale = bias_scale
        self.bias_fn = bias_fn
        self.init_fn = init_fn
        self.updater_activation = updater_activation
        self.reseter_activation = reseter_activation
        self.c_dim = c_dim

        assert rng is not None, "random number generator should not be empty!"

        super(RecurrentLayerWithSearch, self).__init__(self.n_hids,
                self.n_hids, rng, name)

        self.trng = RandomStreams(self.rng.randint(int(1e6)))
        self.params = []
        self._init_params()

    def _init_params(self):
        self.W_hh = theano.shared(
                self.init_fn(self.n_hids,
                self.n_hids,
                -1,
                self.scale,
                rng=self.rng),
                name="W_%s"%self.name)
        self.params = [self.W_hh]
        self.G_hh = theano.shared(
                self.init_fn(self.n_hids,
                    self.n_hids,
                    -1,
                    self.scale,
                    rng=self.rng),
                name="G_%s"%self.name)
        self.params.append(self.G_hh)
        self.R_hh = theano.shared(
                self.init_fn(self.n_hids,
                    self.n_hids,
                    -1,
                    self.scale,
                    rng=self.rng),
                name="R_%s"%self.name)
        self.params.append(self.R_hh)
        self.A_cp = theano.shared(
                sample_weights_classic(self.c_dim,
                    self.n_hids,
                    -1,
                    10 ** (-3),
                    rng=self.rng),
                name="A_%s"%self.name)
        self.params.append(self.A_cp)
        self.B_hp = theano.shared(
                sample_weights_classic(self.n_hids,
                    self.n_hids,
                    -1,
                    10 ** (-3),
                    rng=self.rng),
                name="B_%s"%self.name)
        self.params.append(self.B_hp)
        self.D_pe = theano.shared(
                numpy.zeros((self.n_hids, 1), dtype="float32"),
                name="D_%s"%self.name)
        self.params.append(self.D_pe)
        self.params_grad_scale = [self.grad_scale for x in self.params]
        self.restricted_params = [x for x in self.params]

    def set_decoding_layers(self, c_inputer, c_reseter, c_updater):
        self.c_inputer = c_inputer
        self.c_reseter = c_reseter
        self.c_updater = c_updater
        for layer in [c_inputer, c_reseter, c_updater]:
            self.params += layer.params
            self.params_grad_scale += layer.params_grad_scale

    def step_fprop(self,
                   state_below,
                   state_before,
                   gater_below=None,
                   reseter_below=None,
                   mask=None,
                   c=None,
                   c_mask=None,
                   p_from_c=None,
                   use_noise=True,
                   no_noise_bias=False,
                   step_num=None,
                   return_alignment=False):
        """
        Constructs the computational graph of this layer.

        :type state_below: theano variable
        :param state_below: the input to the layer

        :type mask: None or theano variable
        :param mask: mask describing the length of each sequence in a
            minibatch

        :type state_before: theano variable
        :param state_before: the previous value of the hidden state of the
            layer

        :type updater_below: theano variable
        :param updater_below: the input to the update gate

        :type reseter_below: theano variable
        :param reseter_below: the input to the reset gate

        :type use_noise: bool
        :param use_noise: flag saying if weight noise should be used in
            computing the output of this layer

        :type no_noise_bias: bool
        :param no_noise_bias: flag saying if weight noise should be added to
            the bias as well
        """

        updater_below = gater_below

        W_hh = self.W_hh
        G_hh = self.G_hh
        R_hh = self.R_hh
        A_cp = self.A_cp
        B_hp = self.B_hp
        D_pe = self.D_pe

        # The code works only with 3D tensors
        cndim = c.ndim
        if cndim == 2:
            c = c[:, None, :]

        # Warning: either source_num or target_num should be 1
        # for the following code to make any sense.
        source_len = c.shape[0]
        source_num = c.shape[1]
        target_num = state_before.shape[0]
        dim = self.n_hids

        # Form projection to the tanh layer from the previous hidden state
        # Shape: (source_len, target_num, dim)
        p_from_h = ReplicateLayer(source_len)(utils.dot(state_before, B_hp)).out

        # Form projection to the tanh layer from the source annotation.
        if not p_from_c:
            p_from_c =  utils.dot(c, A_cp).reshape((source_len, source_num, dim))

        # Sum projections - broadcasting happens at the dimension 1.
        p = p_from_h + p_from_c

        # Apply non-linearity and project to energy.
        energy = TT.exp(utils.dot(TT.tanh(p), D_pe)).reshape((source_len, target_num))
        if c_mask:
            # This is used for batches only, that is target_num == source_num
            energy *= c_mask

        # Calculate energy sums.
        normalizer = energy.sum(axis=0)

        # Get probabilities.
        probs = energy / normalizer

        # Calculate weighted sums of source annotations.
        # If target_num == 1, c shoulds broadcasted at the 1st dimension.
        # Probabilities are broadcasted at the 2nd dimension.
        ctx = (c * probs.dimshuffle(0, 1, 'x')).sum(axis=0)

        state_below += self.c_inputer(ctx).out
        reseter_below += self.c_reseter(ctx).out
        updater_below += self.c_updater(ctx).out

        # Reset gate:
        # optionally reset the hidden state.
        reseter = self.reseter_activation(TT.dot(state_before, R_hh) +
                reseter_below)
        reseted_state_before = reseter * state_before

        # Feed the input to obtain potential new state.
        preactiv = TT.dot(reseted_state_before, W_hh) + state_below
        h = self.activation(preactiv)

        # Update gate:
        # optionally reject the potential new state and use the new one.
        updater = self.updater_activation(TT.dot(state_before, G_hh) +
                updater_below)
        h = updater * h + (1-updater) * state_before

        if mask is not None:
            if h.ndim ==2 and mask.ndim==1:
                mask = mask.dimshuffle(0,'x')
            h = mask * h + (1-mask) * state_before

        results = [h, ctx]
        if return_alignment:
            results += [probs]
        return results

    def fprop(self,
              state_below,
              mask=None,
              init_state=None,
              gater_below=None,
              reseter_below=None,
              c=None,
              c_mask=None,
              nsteps=None,
              batch_size=None,
              use_noise=True,
              truncate_gradient=-1,
              no_noise_bias=False,
              return_alignment=False):

        updater_below = gater_below

        if theano.config.floatX=='float32':
            floatX = numpy.float32
        else:
            floatX = numpy.float64
        if nsteps is None:
            nsteps = state_below.shape[0]
            if batch_size and batch_size != 1:
                nsteps = nsteps / batch_size
        if batch_size is None and state_below.ndim == 3:
            batch_size = state_below.shape[1]
        if state_below.ndim == 2 and \
           (not isinstance(batch_size,int) or batch_size > 1):
            state_below = state_below.reshape((nsteps, batch_size, self.n_in))
            if updater_below:
                updater_below = updater_below.reshape((nsteps, batch_size, self.n_in))
            if reseter_below:
                reseter_below = reseter_below.reshape((nsteps, batch_size, self.n_in))

        if not init_state:
            if not isinstance(batch_size, int) or batch_size != 1:
                init_state = TT.alloc(floatX(0), batch_size, self.n_hids)
            else:
                init_state = TT.alloc(floatX(0), self.n_hids)

        if mask:
            sequences = [state_below, mask, updater_below, reseter_below]
            non_sequences = [c_mask]
            fn = lambda x, m, g, r, h, c1, cm, pc : self.step_fprop(x, h, mask=m,
                    gater_below=g, reseter_below=r,
                    c=c1, p_from_c=pc, c_mask=cm,
                    use_noise=use_noise, no_noise_bias=no_noise_bias,
                    return_alignment=return_alignment)
        else:
            sequences = [state_below, updater_below, reseter_below]
            non_sequences = []
            fn = lambda x, g, r, h, c1, pc : self.step_fprop(x, h,
                    gater_below=g, reseter_below=r,
                    c=c1, p_from_c=pc,
                    use_noise=use_noise, no_noise_bias=no_noise_bias,
                    return_alignment=return_alignment)

        p_from_c =  utils.dot(c, self.A_cp).reshape(
                (c.shape[0], c.shape[1], self.n_hids))
        non_sequences = [c] + non_sequences + [p_from_c]

        outputs_info = [init_state, None]
        if return_alignment:
            outputs_info.append(None)

        rval, updates = theano.scan(fn,
                        sequences=sequences,
                        non_sequences=non_sequences,
                        outputs_info=outputs_info,
                        name='layer_%s'%self.name,
                        truncate_gradient=truncate_gradient,
                        n_steps=nsteps)
        self.out = rval
        self.rval = rval
        self.updates = updates

        return self.out

class ReplicateLayer(Layer):

    def __init__(self, n_times):
        self.n_times = n_times
        super(ReplicateLayer, self).__init__(0, 0, None)

    def fprop(self, x):
        # This is black magic based on broadcasting,
        # that's why variable names don't make any sense.
        a = TT.shape_padleft(x)
        padding = [1] * x.ndim
        b = TT.alloc(numpy.float32(1), self.n_times, *padding)
        self.out = a * b
        return self.out

class PadLayer(Layer):

    def __init__(self, required):
        self.required = required
        Layer.__init__(self, 0, 0, None)

    def fprop(self, x):
        if_longer = x[:self.required]
        padding = ReplicateLayer(TT.max([1, self.required - x.shape[0]]))(x[-1]).out
        if_shorter = TT.concatenate([x, padding])
        diff = x.shape[0] - self.required
        self.out = ifelse(diff < 0, if_shorter, if_longer)
        return self.out

class ZeroLayer(Layer):

    def fprop(self, x):
        self.out = TT.zeros(x.shape)
        return self.out

def none_if_zero(x):
    if x == 0:
        return None
    return x

class Maxout(object):

    def __init__(self, maxout_part):
        self.maxout_part = maxout_part

    def __call__(self, x):
        shape = x.shape
        if x.ndim == 1:
            shape1 = TT.cast(shape[0] / self.maxout_part, 'int64')
            shape2 = TT.cast(self.maxout_part, 'int64')
            x = x.reshape([shape1, shape2])
            x = x.max(1)
        else:
            shape1 = TT.cast(shape[1] / self.maxout_part, 'int64')
            shape2 = TT.cast(self.maxout_part, 'int64')
            x = x.reshape([shape[0], shape1, shape2])
            x = x.max(2)
        return x

def prefix_lookup(state, p, s):
    if '%s_%s'%(p,s) in state:
        return state['%s_%s'%(p, s)]
    return state[s]

class EncoderDecoderBase(object):

    def _create_embedding_layers(self):
        logger.debug("_create_embedding_layers")
        self.approx_embedder = MultiLayer(
            self.rng,
            n_in=self.state['n_sym_source']
                if self.prefix.find("enc") >= 0
                else self.state['n_sym_target'],
            n_hids=[self.state['rank_n_approx']],
            activation=[self.state['rank_n_activ']],
            name='{}_approx_embdr'.format(self.prefix),
            **self.default_kwargs)

        # We have 3 embeddings for each word in each level,
        # the one used as input,
        # the one used to control resetting gate,
        # the one used to control update gate.
        self.input_embedders = [lambda x : 0] * self.num_levels
        self.reset_embedders = [lambda x : 0] * self.num_levels
        self.update_embedders = [lambda x : 0] * self.num_levels
        embedder_kwargs = dict(self.default_kwargs)
        embedder_kwargs.update(dict(
            n_in=self.state['rank_n_approx'],
            n_hids=[self.state['dim'] * self.state['dim_mult']],
            activation=['lambda x:x']))
        for level in range(self.num_levels):
            self.input_embedders[level] = MultiLayer(
                self.rng,
                name='{}_input_embdr_{}'.format(self.prefix, level),
                **embedder_kwargs)
            if prefix_lookup(self.state, self.prefix, 'rec_gating'):
                self.update_embedders[level] = MultiLayer(
                    self.rng,
                    learn_bias=False,
                    name='{}_update_embdr_{}'.format(self.prefix, level),
                    **embedder_kwargs)
            if prefix_lookup(self.state, self.prefix, 'rec_reseting'):
                self.reset_embedders[level] =  MultiLayer(
                    self.rng,
                    learn_bias=False,
                    name='{}_reset_embdr_{}'.format(self.prefix, level),
                    **embedder_kwargs)

    def _create_inter_level_layers(self):
        logger.debug("_create_inter_level_layers")
        inter_level_kwargs = dict(self.default_kwargs)
        inter_level_kwargs.update(
                n_in=self.state['dim'],
                n_hids=self.state['dim'] * self.state['dim_mult'],
                activation=['lambda x:x'])

        self.inputers = [0] * self.num_levels
        self.reseters = [0] * self.num_levels
        self.updaters = [0] * self.num_levels
        for level in range(1, self.num_levels):
            self.inputers[level] = MultiLayer(self.rng,
                    name="{}_inputer_{}".format(self.prefix, level),
                    **inter_level_kwargs)
            if prefix_lookup(self.state, self.prefix, 'rec_reseting'):
                self.reseters[level] = MultiLayer(self.rng,
                    name="{}_reseter_{}".format(self.prefix, level),
                    **inter_level_kwargs)
            if prefix_lookup(self.state, self.prefix, 'rec_gating'):
                self.updaters[level] = MultiLayer(self.rng,
                    name="{}_updater_{}".format(self.prefix, level),
                    **inter_level_kwargs)

    def _create_transition_layers(self):
        logger.debug("_create_transition_layers")
        self.transitions = []
        rec_layer = eval(prefix_lookup(self.state, self.prefix, 'rec_layer'))
        add_args = dict()
        if rec_layer == RecurrentLayerWithSearch:
            add_args = dict(c_dim=self.state['c_dim'])
        for level in range(self.num_levels):
            self.transitions.append(rec_layer(
                    self.rng,
                    n_hids=self.state['dim'],
                    activation=prefix_lookup(self.state, self.prefix, 'activ'),
                    bias_scale=self.state['bias'],
                    init_fn=(self.state['rec_weight_init_fn']
                        if not self.skip_init
                        else "sample_zeros"),
                    scale=prefix_lookup(self.state, self.prefix, 'rec_weight_scale'),
                    weight_noise=self.state['weight_noise_rec'],
                    dropout=self.state['dropout_rec'],
                    gating=prefix_lookup(self.state, self.prefix, 'rec_gating'),
                    gater_activation=prefix_lookup(self.state, self.prefix, 'rec_gater'),
                    reseting=prefix_lookup(self.state, self.prefix, 'rec_reseting'),
                    reseter_activation=prefix_lookup(self.state, self.prefix, 'rec_reseter'),
                    name='{}_transition_{}'.format(self.prefix, level),
                    **add_args))

class Encoder(EncoderDecoderBase):

    def __init__(self, state, rng, prefix='enc', skip_init=False):
        self.state = state
        self.rng = rng
        self.prefix = prefix
        self.skip_init = skip_init

        self.num_levels = self.state['encoder_stack']

        # support multiple gating/memory units
        if 'dim_mult' not in self.state:
            self.state['dim_mult'] = 1.
        if 'hid_mult' not in self.state:
            self.state['hid_mult'] = 1.

    def create_layers(self):
        """ Create all elements of Encoder's computation graph"""

        self.default_kwargs = dict(
            init_fn=self.state['weight_init_fn'] if not self.skip_init else "sample_zeros",
            weight_noise=self.state['weight_noise'],
            scale=self.state['weight_scale'])

        self._create_embedding_layers()
        self._create_transition_layers()
        self._create_inter_level_layers()
        self._create_representation_layers()

    def _create_representation_layers(self):
        logger.debug("_create_representation_layers")
        # If we have a stack of RNN, then their last hidden states
        # are combined with a maxout layer.
        self.repr_contributors = [None] * self.num_levels
        for level in range(self.num_levels):
            self.repr_contributors[level] = MultiLayer(
                self.rng,
                n_in=self.state['dim'],
                n_hids=[self.state['dim'] * self.state['maxout_part']],
                activation=['lambda x: x'],
                name="{}_repr_contrib_{}".format(self.prefix, level),
                **self.default_kwargs)
        self.repr_calculator = UnaryOp(
                activation=eval(self.state['unary_activ']),
                name="{}_repr_calc".format(self.prefix))

    def build_encoder(self, x,
            x_mask=None,
            use_noise=False,
            approx_embeddings=None,
            return_hidden_layers=False):
        """Create the computational graph of the RNN Encoder

        :param x:
            input variable, either vector of word indices or
            matrix of word indices, where each column is a sentence

        :param x_mask:
            when x is a matrix and input sequences are
            of variable length, this 1/0 matrix is used to specify
            the matrix positions where the input actually is

        :param use_noise:
            turns on addition of noise to weights
            (UNTESTED)

        :param approx_embeddings:
            forces encoder to use given embeddings instead of its own

        :param return_hidden_layers:
            if True, encoder returns all the activations of the hidden layer
            (WORKS ONLY IN NON-HIERARCHICAL CASE)
        """

        # Low rank embeddings of all the input words.
        # Shape in case of matrix input:
        #   (max_seq_len * batch_size, rank_n_approx),
        #   where max_seq_len is the maximum length of batch sequences.
        # Here and later n_words = max_seq_len * batch_size.
        # Shape in case of vector input:
        #   (seq_len, rank_n_approx)
        if not approx_embeddings:
            approx_embeddings = self.approx_embedder(x)

        # Low rank embeddings are projected to contribute
        # to input, reset and update signals.
        # All the shapes: (n_words, dim)
        input_signals = []
        reset_signals = []
        update_signals = []
        for level in range(self.num_levels):
            input_signals.append(self.input_embedders[level](approx_embeddings))
            update_signals.append(self.update_embedders[level](approx_embeddings))
            reset_signals.append(self.reset_embedders[level](approx_embeddings))

        # Hidden layers.
        # Shape in case of matrix input: (max_seq_len, batch_size, dim)
        # Shape in case of vector input: (seq_len, dim)
        hidden_layers = []
        for level in range(self.num_levels):
            # Each hidden layer (except the bottom one) receives
            # input, reset and update signals from below.
            # All the shapes: (n_words, dim)
            if level > 0:
                input_signals[level] += self.inputers[level](hidden_layers[-1])
                update_signals[level] += self.updaters[level](hidden_layers[-1])
                reset_signals[level] += self.reseters[level](hidden_layers[-1])
            hidden_layers.append(self.transitions[level](
                    input_signals[level],
                    nsteps=x.shape[0],
                    batch_size=x.shape[1] if x.ndim == 2 else 1,
                    mask=x_mask,
                    gater_below=none_if_zero(update_signals[level]),
                    reseter_below=none_if_zero(reset_signals[level]),
                    use_noise=use_noise))
        if return_hidden_layers:
            assert self.state['encoder_stack'] == 1
            return hidden_layers[0]

        # If we no stack of RNN but only a usual one,
        # then the last hidden state is used as a representation.
        # Return value shape in case of matrix input:
        #   (batch_size, dim)
        # Return value shape in case of vector input:
        #   (dim,)
        if self.num_levels == 1 or self.state['take_top']:
            c = LastState()(hidden_layers[-1])
            if c.out.ndim == 2:
                c.out = c.out[:,:self.state['dim']]
            else:
                c.out = c.out[:self.state['dim']]
            return c

        # If we have a stack of RNN, then their last hidden states
        # are combined with a maxout layer.
        # Return value however has the same shape.
        contributions = []
        for level in range(self.num_levels):
            contributions.append(self.repr_contributors[level](
                LastState()(hidden_layers[level])))
        # I do not know a good starting value for sum
        c = self.repr_calculator(sum(contributions[1:], contributions[0]))
        return c

class Decoder(EncoderDecoderBase):

    EVALUATION = 0
    SAMPLING = 1
    BEAM_SEARCH = 2

    def __init__(self, state, rng, prefix='dec',
            skip_init=False, compute_alignment=False):
        self.state = state
        self.rng = rng
        self.prefix = prefix
        self.skip_init = skip_init
        self.compute_alignment = compute_alignment

        # Actually there is a problem here -
        # we don't make difference between number of input layers
        # and outputs layers.
        self.num_levels = self.state['decoder_stack']

        if 'dim_mult' not in self.state:
            self.state['dim_mult'] = 1.

    def create_layers(self):
        """ Create all elements of Decoder's computation graph"""

        self.default_kwargs = dict(
            init_fn=self.state['weight_init_fn'] if not self.skip_init else "sample_zeros",
            weight_noise=self.state['weight_noise'],
            scale=self.state['weight_scale'])

        self._create_embedding_layers()
        self._create_transition_layers()
        self._create_inter_level_layers()
        self._create_initialization_layers()
        self._create_decoding_layers()
        self._create_readout_layers()

        if self.state['search']:
            assert self.num_levels == 1
            self.transitions[0].set_decoding_layers(
                    self.decode_inputers[0],
                    self.decode_reseters[0],
                    self.decode_updaters[0])

    def _create_initialization_layers(self):
        logger.debug("_create_initialization_layers")
        self.initializers = [ZeroLayer()] * self.num_levels
        if self.state['bias_code']:
            for level in range(self.num_levels):
                self.initializers[level] = MultiLayer(
                    self.rng,
                    n_in=self.state['dim'],
                    n_hids=[self.state['dim'] * self.state['hid_mult']],
                    activation=[prefix_lookup(self.state, 'dec', 'activ')],
                    bias_scale=[self.state['bias']],
                    name='{}_initializer_{}'.format(self.prefix, level),
                    **self.default_kwargs)

    def _create_decoding_layers(self):
        logger.debug("_create_decoding_layers")
        self.decode_inputers = [lambda x : 0] * self.num_levels
        self.decode_reseters = [lambda x : 0] * self.num_levels
        self.decode_updaters = [lambda x : 0] * self.num_levels
        self.back_decode_inputers = [lambda x : 0] * self.num_levels
        self.back_decode_reseters = [lambda x : 0] * self.num_levels
        self.back_decode_updaters = [lambda x : 0] * self.num_levels

        decoding_kwargs = dict(self.default_kwargs)
        decoding_kwargs.update(dict(
                n_in=self.state['c_dim'],
                n_hids=self.state['dim'] * self.state['dim_mult'],
                activation=['lambda x:x'],
                learn_bias=False))

        if self.state['decoding_inputs']:
            for level in range(self.num_levels):
                # Input contributions
                self.decode_inputers[level] = MultiLayer(
                    self.rng,
                    name='{}_dec_inputter_{}'.format(self.prefix, level),
                    **decoding_kwargs)
                # Update gate contributions
                if prefix_lookup(self.state, 'dec', 'rec_gating'):
                    self.decode_updaters[level] = MultiLayer(
                        self.rng,
                        name='{}_dec_updater_{}'.format(self.prefix, level),
                        **decoding_kwargs)
                # Reset gate contributions
                if prefix_lookup(self.state, 'dec', 'rec_reseting'):
                    self.decode_reseters[level] = MultiLayer(
                        self.rng,
                        name='{}_dec_reseter_{}'.format(self.prefix, level),
                        **decoding_kwargs)

    def _create_readout_layers(self):
        logger.debug("_create_readout_layers")

        readout_kwargs = dict(self.default_kwargs)
        readout_kwargs.update(dict(
                n_hids=self.state['dim'],
                activation='lambda x: x',
            ))

        self.repr_readout = MultiLayer(
                self.rng,
                n_in=self.state['c_dim'],
                learn_bias=False,
                name='{}_repr_readout'.format(self.prefix),
                **readout_kwargs)

        # Attention - this is the only readout layer
        # with trainable bias. Should be careful with that.
        self.hidden_readouts = [None] * self.num_levels
        for level in range(self.num_levels):
            self.hidden_readouts[level] = MultiLayer(
                self.rng,
                n_in=self.state['dim'],
                name='{}_hid_readout_{}'.format(self.prefix, level),
                **readout_kwargs)

        self.prev_word_readout = 0
        if self.state['bigram']:
            self.prev_word_readout = MultiLayer(
                self.rng,
                n_in=self.state['rank_n_approx'],
                n_hids=self.state['dim'],
                activation=['lambda x:x'],
                learn_bias=False,
                name='{}_prev_readout_{}'.format(self.prefix, level),
                **self.default_kwargs)

        if self.state['deep_out']:
            act_layer = UnaryOp(activation=eval(self.state['unary_activ']))
            drop_layer = DropOp(rng=self.rng, dropout=self.state['dropout'])
            self.output_nonlinearities = [act_layer, drop_layer]
            self.output_layer = SoftmaxLayer(
                    self.rng,
                    self.state['dim'] / self.state['maxout_part'],
                    self.state['n_sym_target'],
                    sparsity=-1,
                    rank_n_approx=self.state['rank_n_approx'],
                    name='{}_deep_softmax'.format(self.prefix),
                    use_nce=self.state['use_nce'] if 'use_nce' in self.state else False,
                    **self.default_kwargs)
        else:
            self.output_nonlinearities = []
            self.output_layer = SoftmaxLayer(
                    self.rng,
                    self.state['dim'],
                    self.state['n_sym_target'],
                    sparsity=-1,
                    rank_n_approx=self.state['rank_n_approx'],
                    name='dec_softmax',
                    sum_over_time=True,
                    use_nce=self.state['use_nce'] if 'use_nce' in self.state else False,
                    **self.default_kwargs)

    def build_decoder(self, c, y,
            c_mask=None,
            y_mask=None,
            step_num=None,
            mode=EVALUATION,
            given_init_states=None,
            T=1):
        """Create the computational graph of the RNN Decoder.

        :param c:
            representations produced by an encoder.
            (n_samples, dim) matrix if mode == sampling or
            (max_seq_len, batch_size, dim) matrix if mode == evaluation

        :param c_mask:
            if mode == evaluation a 0/1 matrix identifying valid positions in c

        :param y:
            if mode == evaluation
                target sequences, matrix of word indices of shape (max_seq_len, batch_size),
                where each column is a sequence
            if mode != evaluation
                a vector of previous words of shape (n_samples,)

        :param y_mask:
            if mode == evaluation a 0/1 matrix determining lengths
                of the target sequences, must be None otherwise

        :param mode:
            chooses on of three modes: evaluation, sampling and beam_search

        :param given_init_states:
            for sampling and beam_search. A list of hidden states
                matrices for each layer, each matrix is (n_samples, dim)

        :param T:
            sampling temperature
        """

        # Check parameter consistency
        if mode == Decoder.EVALUATION:
            assert not given_init_states
        else:
            assert not y_mask
            assert given_init_states
            if mode == Decoder.BEAM_SEARCH:
                assert T == 1

        # For log-likelihood evaluation the representation
        # be replicated for conveniency. In case backward RNN is used
        # it is not done.
        # Shape if mode == evaluation
        #   (max_seq_len, batch_size, dim)
        # Shape if mode != evaluation
        #   (n_samples, dim)
        if not self.state['search']:
            if mode == Decoder.EVALUATION:
                c = PadLayer(y.shape[0])(c)
            else:
                assert step_num
                c_pos = TT.minimum(step_num, c.shape[0] - 1)

        # Low rank embeddings of all the input words.
        # Shape if mode == evaluation
        #   (n_words, rank_n_approx),
        # Shape if mode != evaluation
        #   (n_samples, rank_n_approx)
        approx_embeddings = self.approx_embedder(y)

        # Low rank embeddings are projected to contribute
        # to input, reset and update signals.
        # All the shapes if mode == evaluation:
        #   (n_words, dim)
        # All the shape if mode != evaluation:
        #   (n_samples, dim)
        input_signals = []
        reset_signals = []
        update_signals = []
        for level in range(self.num_levels):
            # Contributions directly from input words.
            input_signals.append(self.input_embedders[level](approx_embeddings))
            update_signals.append(self.update_embedders[level](approx_embeddings))
            reset_signals.append(self.reset_embedders[level](approx_embeddings))

            # Contributions from the encoded source sentence.
            if not self.state['search']:
                current_c = c if mode == Decoder.EVALUATION else c[c_pos]
                input_signals[level] += self.decode_inputers[level](current_c)
                update_signals[level] += self.decode_updaters[level](current_c)
                reset_signals[level] += self.decode_reseters[level](current_c)

        # Hidden layers' initial states.
        # Shapes if mode == evaluation:
        #   (batch_size, dim)
        # Shape if mode != evaluation:
        #   (n_samples, dim)
        init_states = given_init_states
        if not init_states:
            init_states = []
            for level in range(self.num_levels):
                init_c = c[0, :, -self.state['dim']:]
                init_states.append(self.initializers[level](init_c))

        # Hidden layers' states.
        # Shapes if mode == evaluation:
        #  (seq_len, batch_size, dim)
        # Shapes if mode != evaluation:
        #  (n_samples, dim)
        hidden_layers = []
        contexts = []
        # Default value for alignment must be smth computable
        alignment = TT.zeros((1,))
        for level in range(self.num_levels):
            if level > 0:
                input_signals[level] += self.inputers[level](hidden_layers[level - 1])
                update_signals[level] += self.updaters[level](hidden_layers[level - 1])
                reset_signals[level] += self.reseters[level](hidden_layers[level - 1])
            add_kwargs = (dict(state_before=init_states[level])
                        if mode != Decoder.EVALUATION
                        else dict(init_state=init_states[level],
                            batch_size=y.shape[1] if y.ndim == 2 else 1,
                            nsteps=y.shape[0]))
            if self.state['search']:
                add_kwargs['c'] = c
                add_kwargs['c_mask'] = c_mask
                add_kwargs['return_alignment'] = self.compute_alignment
                if mode != Decoder.EVALUATION:
                    add_kwargs['step_num'] = step_num
            result = self.transitions[level](
                    input_signals[level],
                    mask=y_mask,
                    gater_below=none_if_zero(update_signals[level]),
                    reseter_below=none_if_zero(reset_signals[level]),
                    one_step=mode != Decoder.EVALUATION,
                    use_noise=mode == Decoder.EVALUATION,
                    **add_kwargs)
            if self.state['search']:
                if self.compute_alignment:
                    h, ctx, alignment = result
                    if mode == Decoder.EVALUATION:
                        alignment = alignment.out
                else:
                    h, ctx = result
            else:
                h = result
                if mode == Decoder.EVALUATION:
                    ctx = c
                else:
                    ctx = ReplicateLayer(given_init_states[0].shape[0])(c[c_pos]).out
            hidden_layers.append(h)
            contexts.append(ctx)

        # In hidden_layers we do no have the initial state, but we need it.
        # Instead of it we have the last one, which we do not need.
        # So what we do is discard the last one and prepend the initial one.
        if mode == Decoder.EVALUATION:
            for level in range(self.num_levels):
                hidden_layers[level].out = TT.concatenate([
                    TT.shape_padleft(init_states[level].out),
                        hidden_layers[level].out])[:-1]

        # The output representation to be fed in softmax.
        # Shape if mode == evaluation
        #   (n_words, dim_r)
        # Shape if mode != evaluation
        #   (n_samples, dim_r)
        # ... where dim_r depends on 'deep_out' option.
        readout = self.repr_readout(contexts[0])
        for level in range(self.num_levels):
            if mode != Decoder.EVALUATION:
                read_from = init_states[level]
            else:
                read_from = hidden_layers[level]
            read_from_var = read_from if type(read_from) == theano.tensor.TensorVariable else read_from.out
            if read_from_var.ndim == 3:
                read_from_var = read_from_var[:,:,:self.state['dim']]
            else:
                read_from_var = read_from_var[:,:self.state['dim']]
            if type(read_from) != theano.tensor.TensorVariable:
                read_from.out = read_from_var
            else:
                read_from = read_from_var
            readout += self.hidden_readouts[level](read_from)
        if self.state['bigram']:
            if mode != Decoder.EVALUATION:
                check_first_word = (y > 0
                    if self.state['check_first_word']
                    else TT.ones((y.shape[0]), dtype="float32"))
                # padright is necessary as we want to multiply each row with a certain scalar
                readout += TT.shape_padright(check_first_word) * self.prev_word_readout(approx_embeddings).out
            else:
                if y.ndim == 1:
                    readout += Shift()(self.prev_word_readout(approx_embeddings).reshape(
                        (y.shape[0], 1, self.state['dim'])))
                else:
                    # This place needs explanation. When prev_word_readout is applied to
                    # approx_embeddings the resulting shape is
                    # (n_batches * sequence_length, repr_dimensionality). We first
                    # transform it into 3D tensor to shift forward in time. Then
                    # reshape it back.
                    readout += Shift()(self.prev_word_readout(approx_embeddings).reshape(
                        (y.shape[0], y.shape[1], self.state['dim']))).reshape(
                                readout.out.shape)
        for fun in self.output_nonlinearities:
            if isinstance(fun, DropOp):
                readout = fun(readout, use_noise= mode == Decoder.EVALUATION)
            else:
                readout = fun(readout)

        if mode == Decoder.SAMPLING:
            sample = self.output_layer.get_sample(
                    state_below=readout,
                    temp=T)
            # Current SoftmaxLayer.get_cost is stupid,
            # that's why we have to reshape a lot.
            self.output_layer.get_cost(
                    state_below=readout.out,
                    temp=T,
                    target=sample)
            log_prob = self.output_layer.cost_per_sample
            return [sample] + [log_prob] + hidden_layers
        elif mode == Decoder.BEAM_SEARCH:
            return self.output_layer(
                    state_below=readout.out,
                    temp=T).out
        elif mode == Decoder.EVALUATION:
            return (self.output_layer.train(
                    state_below=readout,
                    target=y,
                    mask=y_mask,
                    reg=None),
                    alignment)
        else:
            raise Exception("Unknown mode for build_decoder")


    def sampling_step(self, *args):
        """Implements one step of sampling

        Args are necessary since the number (and the order) of arguments can vary"""

        args = iter(args)

        # Arguments that correspond to scan's "sequences" parameteter:
        step_num = next(args)
        assert step_num.ndim == 0

        # Arguments that correspond to scan's "outputs" parameteter:
        prev_word = next(args)
        assert prev_word.ndim == 1
        # skip the previous word log probability
        assert next(args).ndim == 1
        prev_hidden_states = [next(args) for k in range(self.num_levels)]
        assert prev_hidden_states[0].ndim == 2

        # Arguments that correspond to scan's "non_sequences":
        c = next(args)
        assert c.ndim == 2
        T = next(args)
        assert T.ndim == 0

        decoder_args = dict(given_init_states=prev_hidden_states, T=T, c=c)

        sample, log_prob = self.build_decoder(y=prev_word, step_num=step_num, mode=Decoder.SAMPLING, **decoder_args)[:2]
        hidden_states = self.build_decoder(y=sample, step_num=step_num, mode=Decoder.SAMPLING, **decoder_args)[2:]
        return [sample, log_prob] + hidden_states

    def build_initializers(self, c):
        return [init(c).out for init in self.initializers]

    def build_sampler(self, n_samples, n_steps, T, c):
        states = [TT.zeros(shape=(n_samples,), dtype='int64'),
                TT.zeros(shape=(n_samples,), dtype='float32')]
        init_c = c[0, -self.state['dim']:]
        states += [ReplicateLayer(n_samples)(init(init_c).out).out for init in self.initializers]

        if not self.state['search']:
            c = PadLayer(n_steps)(c).out

        # Pad with final states
        non_sequences = [c, T]

        outputs, updates = theano.scan(self.sampling_step,
                outputs_info=states,
                non_sequences=non_sequences,
                sequences=[TT.arange(n_steps, dtype="int64")],
                n_steps=n_steps,
                name="{}_sampler_scan".format(self.prefix))
        return (outputs[0], outputs[1]), updates

    def build_next_probs_predictor(self, c, step_num, y, init_states):
        return self.build_decoder(c, y, mode=Decoder.BEAM_SEARCH,
                given_init_states=init_states, step_num=step_num)

    def build_next_states_computer(self, c, step_num, y, init_states):
        return self.build_decoder(c, y, mode=Decoder.SAMPLING,
                given_init_states=init_states, step_num=step_num)[2:]

class RNNEncoderDecoder(object):
    """This class encapsulates the translation model.

    The expected usage pattern is:
    >>> encdec = RNNEncoderDecoder(...)
    >>> encdec.build(...)
    >>> useful_smth = encdec.create_useful_smth(...)

    Functions from the create_smth family (except create_lm_model)
    when called complile and return functions that do useful stuff.
    """

    def __init__(self, state, rng,
            skip_init=False,
            compute_alignment=False):
        """Constructor.

        :param state:
            A state in the usual groundhog sense.
        :param rng:
            Random number generator. Something like numpy.random.RandomState(seed).
        :param skip_init:
            If True, all the layers are initialized with zeros. Saves time spent on
            parameter initialization if they are loaded later anyway.
        :param compute_alignment:
            If True, the alignment is returned by the decoder.
        """

        self.state = state
        self.rng = rng
        self.skip_init = skip_init
        self.compute_alignment = compute_alignment

    def build(self):
        logger.debug("Create input variables")
        self.x = TT.lmatrix('x')
        self.x_mask = TT.matrix('x_mask')
        self.y = TT.lmatrix('y')
        self.y_mask = TT.matrix('y_mask')
        self.inputs = [self.x, self.y, self.x_mask, self.y_mask]

        # Annotation for the log-likelihood computation
        training_c_components = []

        logger.debug("Create encoder")
        self.encoder = Encoder(self.state, self.rng,
                prefix="enc",
                skip_init=self.skip_init)
        self.encoder.create_layers()

        logger.debug("Build encoding computation graph")
        forward_training_c = self.encoder.build_encoder(
                self.x, self.x_mask,
                use_noise=True,
                return_hidden_layers=True)

        logger.debug("Create backward encoder")
        self.backward_encoder = Encoder(self.state, self.rng,
                prefix="back_enc",
                skip_init=self.skip_init)
        self.backward_encoder.create_layers()

        logger.debug("Build backward encoding computation graph")
        backward_training_c = self.backward_encoder.build_encoder(
                self.x[::-1],
                self.x_mask[::-1],
                use_noise=True,
                approx_embeddings=self.encoder.approx_embedder(self.x[::-1]),
                return_hidden_layers=True)
        # Reverse time for backward representations.
        backward_training_c.out = backward_training_c.out[::-1]

        if self.state['forward']:
            training_c_components.append(forward_training_c)
        if self.state['last_forward']:
            training_c_components.append(
                    ReplicateLayer(self.x.shape[0])(forward_training_c[-1]))
        if self.state['backward']:
            training_c_components.append(backward_training_c)
        if self.state['last_backward']:
            training_c_components.append(ReplicateLayer(self.x.shape[0])
                    (backward_training_c[0]))
        self.state['c_dim'] = len(training_c_components) * self.state['dim']

        logger.debug("Create decoder")
        self.decoder = Decoder(self.state, self.rng,
                skip_init=self.skip_init, compute_alignment=self.compute_alignment)
        self.decoder.create_layers()
        logger.debug("Build log-likelihood computation graph")
        self.predictions, self.alignment = self.decoder.build_decoder(
                c=Concatenate(axis=2)(*training_c_components), c_mask=self.x_mask,
                y=self.y, y_mask=self.y_mask)

        # Annotation for sampling
        sampling_c_components = []

        logger.debug("Build sampling computation graph")
        self.sampling_x = TT.lvector("sampling_x")
        self.n_samples = TT.lscalar("n_samples")
        self.n_steps = TT.lscalar("n_steps")
        self.T = TT.scalar("T")
        self.forward_sampling_c = self.encoder.build_encoder(
                self.sampling_x,
                return_hidden_layers=True).out
        self.backward_sampling_c = self.backward_encoder.build_encoder(
                self.sampling_x[::-1],
                approx_embeddings=self.encoder.approx_embedder(self.sampling_x[::-1]),
                return_hidden_layers=True).out[::-1]
        if self.state['forward']:
            sampling_c_components.append(self.forward_sampling_c)
        if self.state['last_forward']:
            sampling_c_components.append(ReplicateLayer(self.sampling_x.shape[0])
                    (self.forward_sampling_c[-1]))
        if self.state['backward']:
            sampling_c_components.append(self.backward_sampling_c)
        if self.state['last_backward']:
            sampling_c_components.append(ReplicateLayer(self.sampling_x.shape[0])
                    (self.backward_sampling_c[0]))

        self.sampling_c = Concatenate(axis=1)(*sampling_c_components).out
        (self.sample, self.sample_log_prob), self.sampling_updates =\
            self.decoder.build_sampler(self.n_samples, self.n_steps, self.T,
                    c=self.sampling_c)

        logger.debug("Create auxiliary variables")
        self.c = TT.matrix("c")
        self.step_num = TT.lscalar("step_num")
        self.current_states = [TT.matrix("cur_{}".format(i))
                for i in range(self.decoder.num_levels)]
        self.gen_y = TT.lvector("gen_y")

    def create_lm_model(self):
        if hasattr(self, 'lm_model'):
            return self.lm_model
        self.lm_model = LM_Model(
            cost_layer=self.predictions,
            sample_fn=self.create_sampler(),
            weight_noise_amount=self.state['weight_noise_amount'],
            indx_word=self.state['indx_word_target'],
            indx_word_src=self.state['indx_word'],
            rng=self.rng)
        self.lm_model.load_dict(self.state)
        self.lm_model.name2pos = name2pos(self.lm_model.params)
        logger.debug("Model params:\n{}".format(
            pprint.pformat(sorted([p.name for p in self.lm_model.params]))))
        return self.lm_model

    def create_representation_computer(self):
        if not hasattr(self, "repr_fn"):
            self.repr_fn = theano.function(
                    inputs=[self.sampling_x],
                    outputs=[self.sampling_c],
                    name="repr_fn")
        return self.repr_fn

    def create_initializers(self):
        if not hasattr(self, "init_fn"):
            init_c = self.sampling_c[0, -self.state['dim']:]
            self.init_fn = theano.function(
                    inputs=[self.sampling_c],
                    outputs=self.decoder.build_initializers(init_c),
                    name="init_fn")
        return self.init_fn

    def create_sampler(self, many_samples=False):
        if hasattr(self, 'sample_fn'):
            return self.sample_fn
        logger.debug("Compile sampler")
        self.sample_fn = theano.function(
                inputs=[self.n_samples, self.n_steps, self.T, self.sampling_x],
                outputs=[self.sample, self.sample_log_prob],
                updates=self.sampling_updates,
                name="sample_fn")
        if not many_samples:
            def sampler(*args):
                return map(lambda x : x.squeeze(), self.sample_fn(1, *args))
            return sampler
        return self.sample_fn

    def create_scorer(self, batch=False):
        if not hasattr(self, 'score_fn'):
            logger.debug("Compile scorer")
            self.score_fn = theano.function(
                    inputs=self.inputs,
                    outputs=[-self.predictions.cost_per_sample],
                    name="score_fn")
        if batch:
            return self.score_fn
        def scorer(x, y):
            x_mask = numpy.ones(x.shape[0], dtype="float32")
            y_mask = numpy.ones(y.shape[0], dtype="float32")
            return self.score_fn(x[:, None], y[:, None],
                    x_mask[:, None], y_mask[:, None])
        return scorer

    def create_next_probs_computer(self):
        if not hasattr(self, 'next_probs_fn'):
            self.next_probs_fn = theano.function(
                    inputs=[self.c, self.step_num, self.gen_y] + self.current_states,
                    outputs=[self.decoder.build_next_probs_predictor(
                        self.c, self.step_num, self.gen_y, self.current_states)],
                    name="next_probs_fn")
        return self.next_probs_fn

    def create_next_states_computer(self):
        if not hasattr(self, 'next_states_fn'):
            self.next_states_fn = theano.function(
                    inputs=[self.c, self.step_num, self.gen_y] + self.current_states,
                    outputs=self.decoder.build_next_states_computer(
                        self.c, self.step_num, self.gen_y, self.current_states),
                    name="next_states_fn")
        return self.next_states_fn


    def create_probs_computer(self, return_alignment=False):
        if not hasattr(self, 'probs_fn'):
            logger.debug("Compile probs computer")
            self.probs_fn = theano.function(
                    inputs=self.inputs,
                    outputs=[self.predictions.word_probs, self.alignment],
                    name="probs_fn")
        def probs_computer(x, y):
            x_mask = numpy.ones(x.shape[0], dtype="float32")
            y_mask = numpy.ones(y.shape[0], dtype="float32")
            probs, alignment = self.probs_fn(x[:, None], y[:, None],
                    x_mask[:, None], y_mask[:, None])
            if return_alignment:
                return probs, alignment
            else:
                return probs
        return probs_computer

def parse_input(state, word2idx, line):
    seqin = line.split()
    seqlen = len(seqin)
    seq = numpy.zeros(seqlen+1, dtype='int64')
    unk_sym = state['unk_sym_source']

    for idx,sx in enumerate(seqin):
        seq[idx] = word2idx.get(sx, unk_sym)
    seq[-1] = state['null_sym_source']
    return seq
