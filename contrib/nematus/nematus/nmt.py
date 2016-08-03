'''
Build a neural machine translation model with soft attention
'''
import theano
import theano.tensor as tensor
from theano.sandbox.rng_mrg import MRG_RandomStreams as RandomStreams

import cPickle as pkl
import json
import ipdb
import numpy
import copy

import os
import warnings
import sys
import time

from subprocess import Popen

from collections import OrderedDict

from data_iterator import TextIterator
from util import load_dict
from alignment_util import *

profile = False


# push parameters to Theano shared variables
def zipp(params, tparams):
    for kk, vv in params.iteritems():
        tparams[kk].set_value(vv)


# pull parameters from Theano shared variables
def unzip(zipped):
    new_params = OrderedDict()
    for kk, vv in zipped.iteritems():
        new_params[kk] = vv.get_value()
    return new_params


# get the list of parameters: Note that tparams must be OrderedDict
def itemlist(tparams):
    return [vv for kk, vv in tparams.iteritems()]


# dropout
def dropout_layer(state_before, use_noise, trng):
    proj = tensor.switch(
        use_noise,
        state_before * trng.binomial(state_before.shape, p=0.5, n=1,
                                     dtype=state_before.dtype),
        state_before * 0.5)
    return proj


# dropout that will be re-used at different time steps
def shared_dropout_layer(shape, use_noise, trng, value):
    proj = tensor.switch(
        use_noise,
        trng.binomial(shape, p=value, n=1,
                                     dtype='float32'),
        theano.shared(numpy.float32(value)))
    return proj

# make prefix-appended name
def _p(pp, name):
    return '%s_%s' % (pp, name)


# initialize Theano shared variables according to the initial parameters
def init_tparams(params):
    tparams = OrderedDict()
    for kk, pp in params.iteritems():
        tparams[kk] = theano.shared(params[kk], name=kk)
    return tparams


# load parameters
def load_params(path, params):
    pp = numpy.load(path)
    for kk, vv in params.iteritems():
        if kk not in pp:
            warnings.warn('%s is not in the archive' % kk)
            continue
        params[kk] = pp[kk]

    return params

# layers: 'name': ('parameter initializer', 'feedforward')
layers = {'ff': ('param_init_fflayer', 'fflayer'),
          'gru': ('param_init_gru', 'gru_layer'),
          'gru_cond': ('param_init_gru_cond', 'gru_cond_layer'),
          }


def get_layer(name):
    fns = layers[name]
    return (eval(fns[0]), eval(fns[1]))


# some utilities
def ortho_weight(ndim):
    W = numpy.random.randn(ndim, ndim)
    u, s, v = numpy.linalg.svd(W)
    return u.astype('float32')


def norm_weight(nin, nout=None, scale=0.01, ortho=True):
    if nout is None:
        nout = nin
    if nout == nin and ortho:
        W = ortho_weight(nin)
    else:
        W = scale * numpy.random.randn(nin, nout)
    return W.astype('float32')


def tanh(x):
    return tensor.tanh(x)


def linear(x):
    return x


def concatenate(tensor_list, axis=0):
    """
    Alternative implementation of `theano.tensor.concatenate`.
    This function does exactly the same thing, but contrary to Theano's own
    implementation, the gradient is implemented on the GPU.
    Backpropagating through `theano.tensor.concatenate` yields slowdowns
    because the inverse operation (splitting) needs to be done on the CPU.
    This implementation does not have that problem.
    :usage:
        >>> x, y = theano.tensor.matrices('x', 'y')
        >>> c = concatenate([x, y], axis=1)
    :parameters:
        - tensor_list : list
            list of Theano tensor expressions that should be concatenated.
        - axis : int
            the tensors will be joined along this axis.
    :returns:
        - out : tensor
            the concatenated tensor expression.
    """
    concat_size = sum(tt.shape[axis] for tt in tensor_list)

    output_shape = ()
    for k in range(axis):
        output_shape += (tensor_list[0].shape[k],)
    output_shape += (concat_size,)
    for k in range(axis + 1, tensor_list[0].ndim):
        output_shape += (tensor_list[0].shape[k],)

    out = tensor.zeros(output_shape)
    offset = 0
    for tt in tensor_list:
        indices = ()
        for k in range(axis):
            indices += (slice(None),)
        indices += (slice(offset, offset + tt.shape[axis]),)
        for k in range(axis + 1, tensor_list[0].ndim):
            indices += (slice(None),)

        out = tensor.set_subtensor(out[indices], tt)
        offset += tt.shape[axis]

    return out

# return name of word embedding for factor i
# special handling of factor 0 for backward compatibility
def embedding_name(i):
    if i == 0:
        return 'Wemb'
    else:
        return 'Wemb'+str(i)

# batch preparation
def prepare_data(seqs_x, seqs_y, maxlen=None, n_words_src=30000,
                 n_words=30000):
    # x: a list of sentences
    lengths_x = [len(s) for s in seqs_x]
    lengths_y = [len(s) for s in seqs_y]

    if maxlen is not None:
        new_seqs_x = []
        new_seqs_y = []
        new_lengths_x = []
        new_lengths_y = []
        for l_x, s_x, l_y, s_y in zip(lengths_x, seqs_x, lengths_y, seqs_y):
            if l_x < maxlen and l_y < maxlen:
                new_seqs_x.append(s_x)
                new_lengths_x.append(l_x)
                new_seqs_y.append(s_y)
                new_lengths_y.append(l_y)
        lengths_x = new_lengths_x
        seqs_x = new_seqs_x
        lengths_y = new_lengths_y
        seqs_y = new_seqs_y

        if len(lengths_x) < 1 or len(lengths_y) < 1:
            return None, None, None, None

    n_samples = len(seqs_x)
    n_factors = len(seqs_x[0][0])
    maxlen_x = numpy.max(lengths_x) + 1
    maxlen_y = numpy.max(lengths_y) + 1

    x = numpy.zeros((n_factors, maxlen_x, n_samples)).astype('int64')
    y = numpy.zeros((maxlen_y, n_samples)).astype('int64')
    x_mask = numpy.zeros((maxlen_x, n_samples)).astype('float32')
    y_mask = numpy.zeros((maxlen_y, n_samples)).astype('float32')
    for idx, [s_x, s_y] in enumerate(zip(seqs_x, seqs_y)):
        x[:, :lengths_x[idx], idx] = zip(*s_x)
        x_mask[:lengths_x[idx]+1, idx] = 1.
        y[:lengths_y[idx], idx] = s_y
        y_mask[:lengths_y[idx]+1, idx] = 1.

    return x, x_mask, y, y_mask


# feedforward layer: affine transformation + point-wise nonlinearity
def param_init_fflayer(options, params, prefix='ff', nin=None, nout=None,
                       ortho=True):
    if nin is None:
        nin = options['dim_proj']
    if nout is None:
        nout = options['dim_proj']
    params[_p(prefix, 'W')] = norm_weight(nin, nout, scale=0.01, ortho=ortho)
    params[_p(prefix, 'b')] = numpy.zeros((nout,)).astype('float32')

    return params


def fflayer(tparams, state_below, options, prefix='rconv',
            activ='lambda x: tensor.tanh(x)', **kwargs):
    return eval(activ)(
        tensor.dot(state_below, tparams[_p(prefix, 'W')]) +
        tparams[_p(prefix, 'b')])


# GRU layer
def param_init_gru(options, params, prefix='gru', nin=None, dim=None):
    if nin is None:
        nin = options['dim_proj']
    if dim is None:
        dim = options['dim_proj']

    # embedding to gates transformation weights, biases
    W = numpy.concatenate([norm_weight(nin, dim),
                           norm_weight(nin, dim)], axis=1)
    params[_p(prefix, 'W')] = W
    params[_p(prefix, 'b')] = numpy.zeros((2 * dim,)).astype('float32')

    # recurrent transformation weights for gates
    U = numpy.concatenate([ortho_weight(dim),
                           ortho_weight(dim)], axis=1)
    params[_p(prefix, 'U')] = U

    # embedding to hidden state proposal weights, biases
    Wx = norm_weight(nin, dim)
    params[_p(prefix, 'Wx')] = Wx
    params[_p(prefix, 'bx')] = numpy.zeros((dim,)).astype('float32')

    # recurrent transformation weights for hidden state proposal
    Ux = ortho_weight(dim)
    params[_p(prefix, 'Ux')] = Ux

    return params


def gru_layer(tparams, state_below, options, prefix='gru', mask=None,
              emb_dropout=None,
              rec_dropout=None,
              **kwargs):
    nsteps = state_below.shape[0]
    if state_below.ndim == 3:
        n_samples = state_below.shape[1]
    else:
        n_samples = 1

    dim = tparams[_p(prefix, 'Ux')].shape[1]

    if mask is None:
        mask = tensor.alloc(1., state_below.shape[0], 1)

    # utility function to slice a tensor
    def _slice(_x, n, dim):
        if _x.ndim == 3:
            return _x[:, :, n*dim:(n+1)*dim]
        return _x[:, n*dim:(n+1)*dim]

    # state_below is the input word embeddings
    # input to the gates, concatenated
    state_below_ = tensor.dot(state_below*emb_dropout[0], tparams[_p(prefix, 'W')]) + \
        tparams[_p(prefix, 'b')]
    # input to compute the hidden state proposal
    state_belowx = tensor.dot(state_below*emb_dropout[1], tparams[_p(prefix, 'Wx')]) + \
        tparams[_p(prefix, 'bx')]

    # step function to be used by scan
    # arguments    | sequences |outputs-info| non-seqs
    def _step_slice(m_, x_, xx_, h_, U, Ux, rec_dropout):

        preact = tensor.dot(h_*rec_dropout[0], U)
        preact += x_

        # reset and update gates
        r = tensor.nnet.sigmoid(_slice(preact, 0, dim))
        u = tensor.nnet.sigmoid(_slice(preact, 1, dim))

        # compute the hidden state proposal
        preactx = tensor.dot(h_*rec_dropout[1], Ux)
        preactx = preactx * r
        preactx = preactx + xx_

        # hidden state proposal
        h = tensor.tanh(preactx)

        # leaky integrate and obtain next hidden state
        h = u * h_ + (1. - u) * h
        h = m_[:, None] * h + (1. - m_)[:, None] * h_

        return h

    # prepare scan arguments
    seqs = [mask, state_below_, state_belowx]
    init_states = [tensor.alloc(0., n_samples, dim)]
    _step = _step_slice
    shared_vars = [tparams[_p(prefix, 'U')],
                   tparams[_p(prefix, 'Ux')],
                   rec_dropout]

    rval, updates = theano.scan(_step,
                                sequences=seqs,
                                outputs_info=init_states,
                                non_sequences=shared_vars,
                                name=_p(prefix, '_layers'),
                                n_steps=nsteps,
                                profile=profile,
                                strict=True)
    rval = [rval]
    return rval


# Conditional GRU layer with Attention
def param_init_gru_cond(options, params, prefix='gru_cond',
                        nin=None, dim=None, dimctx=None,
                        nin_nonlin=None, dim_nonlin=None):
    if nin is None:
        nin = options['dim']
    if dim is None:
        dim = options['dim']
    if dimctx is None:
        dimctx = options['dim']
    if nin_nonlin is None:
        nin_nonlin = nin
    if dim_nonlin is None:
        dim_nonlin = dim

    W = numpy.concatenate([norm_weight(nin, dim),
                           norm_weight(nin, dim)], axis=1)
    params[_p(prefix, 'W')] = W
    params[_p(prefix, 'b')] = numpy.zeros((2 * dim,)).astype('float32')
    U = numpy.concatenate([ortho_weight(dim_nonlin),
                           ortho_weight(dim_nonlin)], axis=1)
    params[_p(prefix, 'U')] = U

    Wx = norm_weight(nin_nonlin, dim_nonlin)
    params[_p(prefix, 'Wx')] = Wx
    Ux = ortho_weight(dim_nonlin)
    params[_p(prefix, 'Ux')] = Ux
    params[_p(prefix, 'bx')] = numpy.zeros((dim_nonlin,)).astype('float32')

    U_nl = numpy.concatenate([ortho_weight(dim_nonlin),
                              ortho_weight(dim_nonlin)], axis=1)
    params[_p(prefix, 'U_nl')] = U_nl
    params[_p(prefix, 'b_nl')] = numpy.zeros((2 * dim_nonlin,)).astype('float32')

    Ux_nl = ortho_weight(dim_nonlin)
    params[_p(prefix, 'Ux_nl')] = Ux_nl
    params[_p(prefix, 'bx_nl')] = numpy.zeros((dim_nonlin,)).astype('float32')

    # context to LSTM
    Wc = norm_weight(dimctx, dim*2)
    params[_p(prefix, 'Wc')] = Wc

    Wcx = norm_weight(dimctx, dim)
    params[_p(prefix, 'Wcx')] = Wcx

    # attention: combined -> hidden
    W_comb_att = norm_weight(dim, dimctx)
    params[_p(prefix, 'W_comb_att')] = W_comb_att

    # attention: context -> hidden
    Wc_att = norm_weight(dimctx)
    params[_p(prefix, 'Wc_att')] = Wc_att

    # attention: hidden bias
    b_att = numpy.zeros((dimctx,)).astype('float32')
    params[_p(prefix, 'b_att')] = b_att

    # attention:
    U_att = norm_weight(dimctx, 1)
    params[_p(prefix, 'U_att')] = U_att
    c_att = numpy.zeros((1,)).astype('float32')
    params[_p(prefix, 'c_tt')] = c_att

    return params


def gru_cond_layer(tparams, state_below, options, prefix='gru',
                   mask=None, context=None, one_step=False,
                   init_memory=None, init_state=None,
                   context_mask=None, emb_dropout=None,
                   rec_dropout=None, ctx_dropout=None,
                   **kwargs):

    assert context, 'Context must be provided'

    if one_step:
        assert init_state, 'previous state must be provided'

    nsteps = state_below.shape[0]
    if state_below.ndim == 3:
        n_samples = state_below.shape[1]
    else:
        n_samples = 1

    # mask
    if mask is None:
        mask = tensor.alloc(1., state_below.shape[0], 1)

    dim = tparams[_p(prefix, 'Wcx')].shape[1]

    # initial/previous state
    if init_state is None:
        init_state = tensor.alloc(0., n_samples, dim)

    # projected context
    assert context.ndim == 3, \
        'Context must be 3-d: #annotation x #sample x dim'
    pctx_ = tensor.dot(context*ctx_dropout[0], tparams[_p(prefix, 'Wc_att')]) +\
        tparams[_p(prefix, 'b_att')]

    def _slice(_x, n, dim):
        if _x.ndim == 3:
            return _x[:, :, n*dim:(n+1)*dim]
        return _x[:, n*dim:(n+1)*dim]

    # projected x
    state_belowx = tensor.dot(state_below*emb_dropout[0], tparams[_p(prefix, 'Wx')]) +\
        tparams[_p(prefix, 'bx')]
    state_below_ = tensor.dot(state_below*emb_dropout[1], tparams[_p(prefix, 'W')]) +\
        tparams[_p(prefix, 'b')]

    def _step_slice(m_, x_, xx_, h_, ctx_, alpha_, pctx_, cc_, rec_dropout, ctx_dropout,
                    U, Wc, W_comb_att, U_att, c_tt, Ux, Wcx,
                    U_nl, Ux_nl, b_nl, bx_nl):

        preact1 = tensor.dot(h_*rec_dropout[0], U)
        preact1 += x_
        preact1 = tensor.nnet.sigmoid(preact1)

        r1 = _slice(preact1, 0, dim)
        u1 = _slice(preact1, 1, dim)

        preactx1 = tensor.dot(h_*rec_dropout[1], Ux)
        preactx1 *= r1
        preactx1 += xx_

        h1 = tensor.tanh(preactx1)

        h1 = u1 * h_ + (1. - u1) * h1
        h1 = m_[:, None] * h1 + (1. - m_)[:, None] * h_

        # attention
        pstate_ = tensor.dot(h1*rec_dropout[2], W_comb_att)
        pctx__ = pctx_ + pstate_[None, :, :]
        #pctx__ += xc_
        pctx__ = tensor.tanh(pctx__)
        alpha = tensor.dot(pctx__*ctx_dropout[1], U_att)+c_tt
        alpha = alpha.reshape([alpha.shape[0], alpha.shape[1]])
        alpha = tensor.exp(alpha)
        if context_mask:
            alpha = alpha * context_mask
        alpha = alpha / alpha.sum(0, keepdims=True)
        ctx_ = (cc_ * alpha[:, :, None]).sum(0)  # current context

        preact2 = tensor.dot(h1*rec_dropout[3], U_nl)+b_nl
        preact2 += tensor.dot(ctx_*ctx_dropout[2], Wc)
        preact2 = tensor.nnet.sigmoid(preact2)

        r2 = _slice(preact2, 0, dim)
        u2 = _slice(preact2, 1, dim)

        preactx2 = tensor.dot(h1*rec_dropout[4], Ux_nl)+bx_nl
        preactx2 *= r2
        preactx2 += tensor.dot(ctx_*ctx_dropout[3], Wcx)

        h2 = tensor.tanh(preactx2)

        h2 = u2 * h1 + (1. - u2) * h2
        h2 = m_[:, None] * h2 + (1. - m_)[:, None] * h1

        return h2, ctx_, alpha.T  # pstate_, preact, preactx, r, u

    seqs = [mask, state_below_, state_belowx]
    #seqs = [mask, state_below_, state_belowx, state_belowc]
    _step = _step_slice

    shared_vars = [tparams[_p(prefix, 'U')],
                   tparams[_p(prefix, 'Wc')],
                   tparams[_p(prefix, 'W_comb_att')],
                   tparams[_p(prefix, 'U_att')],
                   tparams[_p(prefix, 'c_tt')],
                   tparams[_p(prefix, 'Ux')],
                   tparams[_p(prefix, 'Wcx')],
                   tparams[_p(prefix, 'U_nl')],
                   tparams[_p(prefix, 'Ux_nl')],
                   tparams[_p(prefix, 'b_nl')],
                   tparams[_p(prefix, 'bx_nl')]]

    if one_step:
        rval = _step(*(seqs + [init_state, None, None, pctx_, context, rec_dropout, ctx_dropout] +
                       shared_vars))
    else:
        rval, updates = theano.scan(_step,
                                    sequences=seqs,
                                    outputs_info=[init_state,
                                                  tensor.alloc(0., n_samples,
                                                               context.shape[2]),
                                                  tensor.alloc(0., n_samples,
                                                               context.shape[0])],
                                    non_sequences=[pctx_, context, rec_dropout, ctx_dropout]+shared_vars,
                                    name=_p(prefix, '_layers'),
                                    n_steps=nsteps,
                                    profile=profile,
                                    strict=True)
    return rval


# initialize all parameters
def init_params(options):
    params = OrderedDict()

    # embedding
    for factor in range(options['factors']):
        params[embedding_name(factor)] = norm_weight(options['n_words_src'], options['dim_per_factor'][factor])

    params['Wemb_dec'] = norm_weight(options['n_words'], options['dim_word'])

    # encoder: bidirectional RNN
    params = get_layer(options['encoder'])[0](options, params,
                                              prefix='encoder',
                                              nin=options['dim_word'],
                                              dim=options['dim'])
    params = get_layer(options['encoder'])[0](options, params,
                                              prefix='encoder_r',
                                              nin=options['dim_word'],
                                              dim=options['dim'])
    ctxdim = 2 * options['dim']

    # init_state, init_cell
    params = get_layer('ff')[0](options, params, prefix='ff_state',
                                nin=ctxdim, nout=options['dim'])
    # decoder
    params = get_layer(options['decoder'])[0](options, params,
                                              prefix='decoder',
                                              nin=options['dim_word'],
                                              dim=options['dim'],
                                              dimctx=ctxdim)
    # readout
    params = get_layer('ff')[0](options, params, prefix='ff_logit_lstm',
                                nin=options['dim'], nout=options['dim_word'],
                                ortho=False)
    params = get_layer('ff')[0](options, params, prefix='ff_logit_prev',
                                nin=options['dim_word'],
                                nout=options['dim_word'], ortho=False)
    params = get_layer('ff')[0](options, params, prefix='ff_logit_ctx',
                                nin=ctxdim, nout=options['dim_word'],
                                ortho=False)
    params = get_layer('ff')[0](options, params, prefix='ff_logit',
                                nin=options['dim_word'],
                                nout=options['n_words'])

    return params


# build a training model
def build_model(tparams, options):
    opt_ret = dict()

    trng = RandomStreams(1234)
    use_noise = theano.shared(numpy.float32(0.))

    # description string: #words x #samples
    x = tensor.tensor3('x', dtype='int64')
    x.tag.test_value = (numpy.random.rand(1, 5, 10)*100).astype('int64')
    x_mask = tensor.matrix('x_mask', dtype='float32')
    x_mask.tag.test_value = numpy.ones(shape=(5, 10)).astype('float32')
    y = tensor.matrix('y', dtype='int64')
    y.tag.test_value = (numpy.random.rand(8, 10)*100).astype('int64')
    y_mask = tensor.matrix('y_mask', dtype='float32')
    y_mask.tag.test_value = numpy.ones(shape=(8, 10)).astype('float32')

    # for the backward rnn, we just need to invert x and x_mask
    xr = x[:,::-1]
    xr_mask = x_mask[::-1]

    n_timesteps = x.shape[1]
    n_timesteps_trg = y.shape[0]
    n_samples = x.shape[2]

    if options['use_dropout']:
        retain_probability_emb = 1-options['dropout_embedding']
        retain_probability_hidden = 1-options['dropout_hidden']
        retain_probability_source = 1-options['dropout_source']
        retain_probability_target = 1-options['dropout_target']
        rec_dropout = shared_dropout_layer((2, n_samples, options['dim']), use_noise, trng, retain_probability_hidden)
        rec_dropout_r = shared_dropout_layer((2, n_samples, options['dim']), use_noise, trng, retain_probability_hidden)
        rec_dropout_d = shared_dropout_layer((5, n_samples, options['dim']), use_noise, trng, retain_probability_hidden)
        emb_dropout = shared_dropout_layer((2, n_samples, options['dim_word']), use_noise, trng, retain_probability_emb)
        emb_dropout_r = shared_dropout_layer((2, n_samples, options['dim_word']), use_noise, trng, retain_probability_emb)
        emb_dropout_d = shared_dropout_layer((2, n_samples, options['dim_word']), use_noise, trng, retain_probability_emb)
        ctx_dropout_d = shared_dropout_layer((4, n_samples, 2*options['dim']), use_noise, trng, retain_probability_hidden)
        source_dropout = shared_dropout_layer((n_timesteps, n_samples, 1), use_noise, trng, retain_probability_source)
        target_dropout = shared_dropout_layer((n_timesteps_trg, n_samples, 1), use_noise, trng, retain_probability_target)
        source_dropout = tensor.tile(source_dropout, (1,1,options['dim_word']))
        target_dropout = tensor.tile(target_dropout, (1,1,options['dim_word']))
    else:
        rec_dropout = theano.shared(numpy.array([1.]*2, dtype='float32'))
        rec_dropout_r = theano.shared(numpy.array([1.]*2, dtype='float32'))
        rec_dropout_d = theano.shared(numpy.array([1.]*5, dtype='float32'))
        emb_dropout = theano.shared(numpy.array([1.]*2, dtype='float32'))
        emb_dropout_r = theano.shared(numpy.array([1.]*2, dtype='float32'))
        emb_dropout_d = theano.shared(numpy.array([1.]*2, dtype='float32'))
        ctx_dropout_d = theano.shared(numpy.array([1.]*4, dtype='float32'))

    # word embedding for forward rnn (source)
    emb = []
    for factor in range(options['factors']):
        emb.append(tparams[embedding_name(factor)][x[factor].flatten()])
    emb = concatenate(emb, axis=1)
    emb = emb.reshape([n_timesteps, n_samples, options['dim_word']])
    if options['use_dropout']:
        emb *= source_dropout

    proj = get_layer(options['encoder'])[1](tparams, emb, options,
                                            prefix='encoder',
                                            mask=x_mask,
                                            emb_dropout=emb_dropout, 
                                            rec_dropout=rec_dropout)
    

    # word embedding for backward rnn (source)
    embr = []
    for factor in range(options['factors']):
        embr.append(tparams[embedding_name(factor)][xr[factor].flatten()])
    embr = concatenate(embr, axis=1)
    embr = embr.reshape([n_timesteps, n_samples, options['dim_word']])
    if options['use_dropout']:
        embr *= source_dropout[::-1]

    projr = get_layer(options['encoder'])[1](tparams, embr, options,
                                             prefix='encoder_r',
                                             mask=xr_mask,
                                             emb_dropout=emb_dropout_r,
                                             rec_dropout=rec_dropout_r)

    # context will be the concatenation of forward and backward rnns
    ctx = concatenate([proj[0], projr[0][::-1]], axis=proj[0].ndim-1)

    # mean of the context (across time) will be used to initialize decoder rnn
    ctx_mean = (ctx * x_mask[:, :, None]).sum(0) / x_mask.sum(0)[:, None]

    # or you can use the last state of forward + backward encoder rnns
    # ctx_mean = concatenate([proj[0][-1], projr[0][-1]], axis=proj[0].ndim-2)

    if options['use_dropout']:
        ctx_mean *= shared_dropout_layer((n_samples, 2*options['dim']), use_noise, trng, retain_probability_hidden)

    # initial decoder state
    init_state = get_layer('ff')[1](tparams, ctx_mean, options,
                                    prefix='ff_state', activ='tanh')

    # word embedding (target), we will shift the target sequence one time step
    # to the right. This is done because of the bi-gram connections in the
    # readout and decoder rnn. The first target will be all zeros and we will
    # not condition on the last output.
    emb = tparams['Wemb_dec'][y.flatten()]
    emb = emb.reshape([n_timesteps_trg, n_samples, options['dim_word']])

    emb_shifted = tensor.zeros_like(emb)
    emb_shifted = tensor.set_subtensor(emb_shifted[1:], emb[:-1])
    emb = emb_shifted

    if options['use_dropout']:
        emb *= target_dropout

    # decoder - pass through the decoder conditional gru with attention
    proj = get_layer(options['decoder'])[1](tparams, emb, options,
                                            prefix='decoder',
                                            mask=y_mask, context=ctx,
                                            context_mask=x_mask,
                                            one_step=False,
                                            init_state=init_state,
                                            emb_dropout=emb_dropout_d,
                                            ctx_dropout=ctx_dropout_d,
                                            rec_dropout=rec_dropout_d)
    # hidden states of the decoder gru
    proj_h = proj[0]

    # weighted averages of context, generated by attention module
    ctxs = proj[1]

    if options['use_dropout']:
        proj_h *= shared_dropout_layer((n_samples, options['dim']), use_noise, trng, retain_probability_hidden)
        emb *= shared_dropout_layer((n_samples, options['dim_word']), use_noise, trng, retain_probability_emb)
        ctxs *= shared_dropout_layer((n_samples, 2*options['dim']), use_noise, trng, retain_probability_hidden)

    # weights (alignment matrix) #####LIUCAN: this is where the attention vector is.
    opt_ret['dec_alphas'] = proj[2]

    # compute word probabilities
    logit_lstm = get_layer('ff')[1](tparams, proj_h, options,
                                    prefix='ff_logit_lstm', activ='linear')
    logit_prev = get_layer('ff')[1](tparams, emb, options,
                                    prefix='ff_logit_prev', activ='linear')
    logit_ctx = get_layer('ff')[1](tparams, ctxs, options,
                                   prefix='ff_logit_ctx', activ='linear')
    logit = tensor.tanh(logit_lstm+logit_prev+logit_ctx)

    if options['use_dropout']:
        logit *= shared_dropout_layer((n_samples, options['dim_word']), use_noise, trng, retain_probability_hidden)

    logit = get_layer('ff')[1](tparams, logit, options,
                               prefix='ff_logit', activ='linear')
    logit_shp = logit.shape
    probs = tensor.nnet.softmax(logit.reshape([logit_shp[0]*logit_shp[1],
                                               logit_shp[2]]))

    # cost
    y_flat = y.flatten()
    y_flat_idx = tensor.arange(y_flat.shape[0]) * options['n_words'] + y_flat
    cost = -tensor.log(probs.flatten()[y_flat_idx])
    cost = cost.reshape([y.shape[0], y.shape[1]])
    cost = (cost * y_mask).sum(0)

    #print "Print out in build_model()"
    #print opt_ret
    return trng, use_noise, x, x_mask, y, y_mask, opt_ret, cost


# build a sampler
def build_sampler(tparams, options, use_noise, trng, return_alignment=False):
    x = tensor.tensor3('x', dtype='int64')
    xr = x[:,::-1]
    n_timesteps = x.shape[1]
    n_samples = x.shape[2]

    # word embedding (source), forward and backward
    emb = []
    embr = []
    for factor in range(options['factors']):
        emb.append(tparams[embedding_name(factor)][x[factor].flatten()])
        embr.append(tparams[embedding_name(factor)][xr[factor].flatten()])
    emb = concatenate(emb, axis=1)
    embr = concatenate(embr, axis=1)
    emb = emb.reshape([n_timesteps, n_samples, options['dim_word']])
    embr = embr.reshape([n_timesteps, n_samples, options['dim_word']])

    if options['use_dropout']:
        retain_probability_emb = 1-options['dropout_embedding']
        retain_probability_hidden = 1-options['dropout_hidden']
        retain_probability_source = 1-options['dropout_source']
        retain_probability_target = 1-options['dropout_target']
        rec_dropout = theano.shared(numpy.array([retain_probability_hidden]*2, dtype='float32'))
        rec_dropout_r = theano.shared(numpy.array([retain_probability_hidden]*2, dtype='float32'))
        rec_dropout_d = theano.shared(numpy.array([retain_probability_hidden]*5, dtype='float32'))
        emb_dropout = theano.shared(numpy.array([retain_probability_emb]*2, dtype='float32'))
        emb_dropout_r = theano.shared(numpy.array([retain_probability_emb]*2, dtype='float32'))
        emb_dropout_d = theano.shared(numpy.array([retain_probability_emb]*2, dtype='float32'))
        ctx_dropout_d = theano.shared(numpy.array([retain_probability_hidden]*4, dtype='float32'))
        source_dropout = theano.shared(numpy.float32(retain_probability_source))
        target_dropout = theano.shared(numpy.float32(retain_probability_target))
        emb *= source_dropout
        embr *= source_dropout
    else:
        rec_dropout = theano.shared(numpy.array([1.]*2, dtype='float32'))
        rec_dropout_r = theano.shared(numpy.array([1.]*2, dtype='float32'))
        rec_dropout_d = theano.shared(numpy.array([1.]*5, dtype='float32'))
        emb_dropout = theano.shared(numpy.array([1.]*2, dtype='float32'))
        emb_dropout_r = theano.shared(numpy.array([1.]*2, dtype='float32'))
        emb_dropout_d = theano.shared(numpy.array([1.]*2, dtype='float32'))
        ctx_dropout_d = theano.shared(numpy.array([1.]*4, dtype='float32'))

    # encoder
    proj = get_layer(options['encoder'])[1](tparams, emb, options,
                                            prefix='encoder', emb_dropout=emb_dropout, rec_dropout=rec_dropout)


    projr = get_layer(options['encoder'])[1](tparams, embr, options,
                                             prefix='encoder_r', emb_dropout=emb_dropout_r, rec_dropout=rec_dropout_r)

    # concatenate forward and backward rnn hidden states
    ctx = concatenate([proj[0], projr[0][::-1]], axis=proj[0].ndim-1)

    # get the input for decoder rnn initializer mlp
    ctx_mean = ctx.mean(0)
    # ctx_mean = concatenate([proj[0][-1],projr[0][-1]], axis=proj[0].ndim-2)

    if options['use_dropout']:
        ctx_mean *= retain_probability_hidden

    init_state = get_layer('ff')[1](tparams, ctx_mean, options,
                                    prefix='ff_state', activ='tanh')

    print >>sys.stderr, 'Building f_init...',
    outs = [init_state, ctx]
    f_init = theano.function([x], outs, name='f_init', profile=profile)
    print >>sys.stderr, 'Done'

    # x: 1 x 1
    y = tensor.vector('y_sampler', dtype='int64')
    init_state = tensor.matrix('init_state', dtype='float32')

    # if it's the first word, emb should be all zero and it is indicated by -1
    emb = tensor.switch(y[:, None] < 0,
                        tensor.alloc(0., 1, tparams['Wemb_dec'].shape[1]),
                        tparams['Wemb_dec'][y])

    if options['use_dropout']:
        emb *= target_dropout

    # apply one step of conditional gru with attention
    proj = get_layer(options['decoder'])[1](tparams, emb, options,
                                            prefix='decoder',
                                            mask=None, context=ctx,
                                            one_step=True,
                                            init_state=init_state,
                                            emb_dropout=emb_dropout_d,
                                            ctx_dropout=ctx_dropout_d,
                                            rec_dropout=rec_dropout_d)
    # get the next hidden state
    next_state = proj[0]

    # get the weighted averages of context for this target word y
    ctxs = proj[1]

    # alignment matrix (attention model)
    dec_alphas = proj[2]

    if options['use_dropout']:
        next_state_up = next_state * retain_probability_hidden
        emb *= retain_probability_emb
        ctxs *= retain_probability_hidden
    else:
        next_state_up = next_state

    logit_lstm = get_layer('ff')[1](tparams, next_state_up, options,
                                    prefix='ff_logit_lstm', activ='linear')
    logit_prev = get_layer('ff')[1](tparams, emb, options,
                                    prefix='ff_logit_prev', activ='linear')
    logit_ctx = get_layer('ff')[1](tparams, ctxs, options,
                                   prefix='ff_logit_ctx', activ='linear')
    logit = tensor.tanh(logit_lstm+logit_prev+logit_ctx)

    if options['use_dropout']:
        logit *= retain_probability_hidden

    logit = get_layer('ff')[1](tparams, logit, options,
                              prefix='ff_logit', activ='linear')

    # compute the softmax probability
    next_probs = tensor.nnet.softmax(logit)

    # sample from softmax distribution to get the sample
    next_sample = trng.multinomial(pvals=next_probs).argmax(1)

    # compile a function to do the whole thing above, next word probability,
    # sampled word for the next target, next hidden state to be used
    print >>sys.stderr, 'Building f_next..',
    inps = [y, ctx, init_state]
    outs = [next_probs, next_sample, next_state]

    if return_alignment:
        outs.append(dec_alphas)

    f_next = theano.function(inps, outs, name='f_next', profile=profile)
    print >>sys.stderr, 'Done'

    return f_init, f_next

# generate sample, either with stochastic sampling or beam search. Note that,
# this function iteratively calls f_init and f_next functions.
def gen_sample(f_init, f_next, x, trng=None, k=1, maxlen=30,
               stochastic=True, argmax=False, return_alignment=False, suppress_unk=False):

    # k is the beam size we have
    if k > 1:
        assert not stochastic, \
            'Beam search does not support stochastic sampling'

    sample = []
    sample_score = []
    sample_word_probs = []
    alignment = []
    if stochastic:
        sample_score = 0

    live_k = 1
    dead_k = 0

    hyp_samples = [[]] * live_k
    word_probs = [[]] * live_k
    hyp_scores = numpy.zeros(live_k).astype('float32')
    hyp_states = []
    if return_alignment:
        hyp_alignment = [[] for _ in xrange(live_k)]

    # for ensemble decoding, we keep track of states and probability distribution
    # for each model in the ensemble
    num_models = len(f_init)
    next_state = [None]*num_models
    ctx0 = [None]*num_models
    next_p = [None]*num_models
    dec_alphas = [None]*num_models
    # get initial state of decoder rnn and encoder context
    for i in xrange(num_models):
        ret = f_init[i](x)
        next_state[i] = ret[0]
        ctx0[i] = ret[1]
    next_w = -1 * numpy.ones((1,)).astype('int64')  # bos indicator

    # x is a sequence of word ids followed by 0, eos id
    for ii in xrange(maxlen):
        for i in xrange(num_models):
            ctx = numpy.tile(ctx0[i], [live_k, 1])
            inps = [next_w, ctx, next_state[i]]
            ret = f_next[i](*inps)
            # dimension of dec_alpha (k-beam-size, number-of-input-hidden-units)
            next_p[i], next_w_tmp, next_state[i] = ret[0], ret[1], ret[2]
            if return_alignment:
                dec_alphas[i] = ret[3]

            if suppress_unk:
                next_p[i][:,1] = -numpy.inf
        if stochastic:
            if argmax:
                nw = sum(next_p)[0].argmax()
            else:
                nw = next_w_tmp[0]
            sample.append(nw)
            sample_score += next_p[0][0, nw]
            if nw == 0:
                break
        else:
            cand_scores = hyp_scores[:, None] - sum(numpy.log(next_p))
            probs = sum(next_p)/num_models
            cand_flat = cand_scores.flatten()
            probs_flat = probs.flatten()
            ranks_flat = cand_flat.argpartition(k-dead_k-1)[:(k-dead_k)]

            #averaging the attention weights accross models
            if return_alignment:
                mean_alignment = sum(dec_alphas)/num_models

            voc_size = next_p[0].shape[1]
            # index of each k-best hypothesis
            trans_indices = ranks_flat / voc_size
            word_indices = ranks_flat % voc_size
            costs = cand_flat[ranks_flat]

            new_hyp_samples = []
            new_hyp_scores = numpy.zeros(k-dead_k).astype('float32')
            new_word_probs = []
            new_hyp_states = []
            if return_alignment:
                # holds the history of attention weights for each time step for each of the surviving hypothesis
                # dimensions (live_k * target_words * source_hidden_units]
                # at each time step we append the attention weights corresponding to the current target word
                new_hyp_alignment = [[] for _ in xrange(k-dead_k)]

            # ti -> index of k-best hypothesis
            for idx, [ti, wi] in enumerate(zip(trans_indices, word_indices)):
                new_hyp_samples.append(hyp_samples[ti]+[wi])
                new_word_probs.append(word_probs[ti] + [probs_flat[ranks_flat[idx]].tolist()])
                new_hyp_scores[idx] = copy.copy(costs[idx])
                new_hyp_states.append([copy.copy(next_state[i][ti]) for i in xrange(num_models)])
                if return_alignment:
                    # get history of attention weights for the current hypothesis
                    new_hyp_alignment[idx] = copy.copy(hyp_alignment[ti])
                    # extend the history with current attention weights
                    new_hyp_alignment[idx].append(mean_alignment[ti])


            # check the finished samples
            new_live_k = 0
            hyp_samples = []
            hyp_scores = []
            hyp_states = []
            word_probs = []
            if return_alignment:
                hyp_alignment = []

            # sample and sample_score hold the k-best translations and their scores
            for idx in xrange(len(new_hyp_samples)):
                if new_hyp_samples[idx][-1] == 0:
                    sample.append(new_hyp_samples[idx])
                    sample_score.append(new_hyp_scores[idx])
                    sample_word_probs.append(new_word_probs[idx])
                    if return_alignment:
                        alignment.append(new_hyp_alignment[idx])
                    dead_k += 1
                else:
                    new_live_k += 1
                    hyp_samples.append(new_hyp_samples[idx])
                    hyp_scores.append(new_hyp_scores[idx])
                    hyp_states.append(new_hyp_states[idx])
                    word_probs.append(new_word_probs[idx])
                    if return_alignment:
                        hyp_alignment.append(new_hyp_alignment[idx])
            hyp_scores = numpy.array(hyp_scores)

            live_k = new_live_k

            if new_live_k < 1:
                break
            if dead_k >= k:
                break

            next_w = numpy.array([w[-1] for w in hyp_samples])
            next_state = [numpy.array(state) for state in zip(*hyp_states)]

    if not stochastic:
        # dump every remaining one
        if live_k > 0:
            for idx in xrange(live_k):
                sample.append(hyp_samples[idx])
                sample_score.append(hyp_scores[idx])
                sample_word_probs.append(word_probs[idx])
                if return_alignment:
                    alignment.append(hyp_alignment[idx])

    if not return_alignment:
        alignment = [None for i in range(len(sample))]

    return sample, sample_score, sample_word_probs, alignment


# calculate the log probablities on a given corpus using translation model
def pred_probs(f_log_probs, prepare_data, options, iterator, verbose=True, normalize=False, alignweights=False):
    probs = []
    n_done = 0

    alignments_json = []

    for x, y in iterator:
        #ensure consistency in number of factors
        if len(x[0][0]) != options['factors']:
            sys.stderr.write('Error: mismatch between number of factors in settings ({0}), and number in validation corpus ({1})\n'.format(options['factors'], len(x[0][0])))
            sys.exit(1)

        n_done += len(x)

        x, x_mask, y, y_mask = prepare_data(x, y,
                                            n_words_src=options['n_words_src'],
                                            n_words=options['n_words'])

        ### in optional save weights mode.
        if alignweights:
            pprobs, attention = f_log_probs(x, x_mask, y, y_mask)
            for jdata in get_alignments(attention, x_mask, y_mask):
                alignments_json.append(jdata)
        else:
            pprobs = f_log_probs(x, x_mask, y, y_mask)

        # normalize scores according to output length
        if normalize:
            lengths = numpy.array([numpy.count_nonzero(s) for s in y_mask.T])
            pprobs /= lengths

        for pp in pprobs:
            probs.append(pp)

        if numpy.isnan(numpy.mean(probs)):
            ipdb.set_trace()

        if verbose:
            print >>sys.stderr, '%d samples computed' % (n_done)

    return numpy.array(probs), alignments_json


# optimizers
# name(hyperp, tparams, grads, inputs (list), cost) = f_grad_shared, f_update
def adam(lr, tparams, grads, inp, cost, beta1=0.9, beta2=0.999, e=1e-8):

    gshared = [theano.shared(p.get_value() * 0., name='%s_grad' % k)
               for k, p in tparams.iteritems()]
    gsup = [(gs, g) for gs, g in zip(gshared, grads)]

    f_grad_shared = theano.function(inp, cost, updates=gsup, profile=profile)

    updates = []

    t_prev = theano.shared(numpy.float32(0.))
    t = t_prev + 1.
    lr_t = lr * tensor.sqrt(1. - beta2**t) / (1. - beta1**t)

    for p, g in zip(tparams.values(), gshared):
        m = theano.shared(p.get_value() * 0., p.name + '_mean')
        v = theano.shared(p.get_value() * 0., p.name + '_variance')
        m_t = beta1 * m + (1. - beta1) * g
        v_t = beta2 * v + (1. - beta2) * g**2
        step = lr_t * m_t / (tensor.sqrt(v_t) + e)
        p_t = p - step
        updates.append((m, m_t))
        updates.append((v, v_t))
        updates.append((p, p_t))
    updates.append((t_prev, t))

    f_update = theano.function([lr], [], updates=updates,
                               on_unused_input='ignore', profile=profile)

    return f_grad_shared, f_update

def adadelta(lr, tparams, grads, inp, cost):
    zipped_grads = [theano.shared(p.get_value() * numpy.float32(0.),
                                  name='%s_grad' % k)
                    for k, p in tparams.iteritems()]
    running_up2 = [theano.shared(p.get_value() * numpy.float32(0.),
                                 name='%s_rup2' % k)
                   for k, p in tparams.iteritems()]
    running_grads2 = [theano.shared(p.get_value() * numpy.float32(0.),
                                    name='%s_rgrad2' % k)
                      for k, p in tparams.iteritems()]

    zgup = [(zg, g) for zg, g in zip(zipped_grads, grads)]
    rg2up = [(rg2, 0.95 * rg2 + 0.05 * (g ** 2))
             for rg2, g in zip(running_grads2, grads)]

    f_grad_shared = theano.function(inp, cost, updates=zgup+rg2up,
                                    profile=profile)

    updir = [-tensor.sqrt(ru2 + 1e-6) / tensor.sqrt(rg2 + 1e-6) * zg
             for zg, ru2, rg2 in zip(zipped_grads, running_up2,
                                     running_grads2)]
    ru2up = [(ru2, 0.95 * ru2 + 0.05 * (ud ** 2))
             for ru2, ud in zip(running_up2, updir)]
    param_up = [(p, p + ud) for p, ud in zip(itemlist(tparams), updir)]

    f_update = theano.function([lr], [], updates=ru2up+param_up,
                               on_unused_input='ignore', profile=profile)

    return f_grad_shared, f_update


def rmsprop(lr, tparams, grads, inp, cost):
    zipped_grads = [theano.shared(p.get_value() * numpy.float32(0.),
                                  name='%s_grad' % k)
                    for k, p in tparams.iteritems()]
    running_grads = [theano.shared(p.get_value() * numpy.float32(0.),
                                   name='%s_rgrad' % k)
                     for k, p in tparams.iteritems()]
    running_grads2 = [theano.shared(p.get_value() * numpy.float32(0.),
                                    name='%s_rgrad2' % k)
                      for k, p in tparams.iteritems()]

    zgup = [(zg, g) for zg, g in zip(zipped_grads, grads)]
    rgup = [(rg, 0.95 * rg + 0.05 * g) for rg, g in zip(running_grads, grads)]
    rg2up = [(rg2, 0.95 * rg2 + 0.05 * (g ** 2))
             for rg2, g in zip(running_grads2, grads)]

    f_grad_shared = theano.function(inp, cost, updates=zgup+rgup+rg2up,
                                    profile=profile)

    updir = [theano.shared(p.get_value() * numpy.float32(0.),
                           name='%s_updir' % k)
             for k, p in tparams.iteritems()]
    updir_new = [(ud, 0.9 * ud - 1e-4 * zg / tensor.sqrt(rg2 - rg ** 2 + 1e-4))
                 for ud, zg, rg, rg2 in zip(updir, zipped_grads, running_grads,
                                            running_grads2)]
    param_up = [(p, p + udn[1])
                for p, udn in zip(itemlist(tparams), updir_new)]
    f_update = theano.function([lr], [], updates=updir_new+param_up,
                               on_unused_input='ignore', profile=profile)

    return f_grad_shared, f_update


def sgd(lr, tparams, grads, inp, cost):
    gshared = [theano.shared(p.get_value() * 0.,
                             name='%s_grad' % k)
               for k, p in tparams.iteritems()]
    gsup = [(gs, g) for gs, g in zip(gshared, grads)]

    f_grad_shared = theano.function(inp, cost, updates=gsup,
                                    profile=profile)

    pup = [(p, p - lr * g) for p, g in zip(itemlist(tparams), gshared)]
    f_update = theano.function([lr], [], updates=pup, profile=profile)

    return f_grad_shared, f_update

def train(dim_word=100,  # word vector dimensionality
          dim=1000,  # the number of LSTM units
          factors=1, # input factors
          dim_per_factor=None, # list of word vector dimensionalities (one per factor): [250,200,50] for total dimensionality of 500
          encoder='gru',
          decoder='gru_cond',
          patience=10,  # early stopping patience
          max_epochs=5000,
          finish_after=10000000,  # finish after this many updates
          dispFreq=100,
          decay_c=0.,  # L2 regularization penalty
          alpha_c=0.,  # alignment regularization
          clip_c=-1.,  # gradient clipping threshold
          lrate=0.01,  # learning rate
          n_words_src=100000,  # source vocabulary size
          n_words=100000,  # target vocabulary size
          maxlen=100,  # maximum length of the description
          optimizer='rmsprop',
          batch_size=16,
          valid_batch_size=16,
          saveto='model.npz',
          validFreq=1000,
          saveFreq=1000,   # save the parameters after every saveFreq updates
          sampleFreq=100,   # generate some samples after every sampleFreq
          datasets=[
              '/data/lisatmp3/chokyun/europarl/europarl-v7.fr-en.en.tok',
              '/data/lisatmp3/chokyun/europarl/europarl-v7.fr-en.fr.tok'],
          valid_datasets=['../data/dev/newstest2011.en.tok',
                          '../data/dev/newstest2011.fr.tok'],
          dictionaries=[
              '/data/lisatmp3/chokyun/europarl/europarl-v7.fr-en.en.tok.pkl',
              '/data/lisatmp3/chokyun/europarl/europarl-v7.fr-en.fr.tok.pkl'],
          use_dropout=False,
          dropout_embedding=0.2, # dropout for input embeddings (0: no dropout)
          dropout_hidden=0.5, # dropout for hidden layers (0: no dropout)
          dropout_source=0, # dropout source words (0: no dropout)
          dropout_target=0, # dropout target words (0: no dropout)
          reload_=False,
          overwrite=False,
          external_validation_script=None,
          shuffle_each_epoch=True,
          finetune=False,
          maxibatch_size=20): #How many minibatches to load at one time

    # Model options
    model_options = locals().copy()


    if model_options['dim_per_factor'] == None:
        if factors == 1:
            model_options['dim_per_factor'] = [model_options['dim_word']]
        else:
            sys.stderr.write('Error: if using factored input, you must specify \'dim_per_factor\'\n')
            sys.exit(1)

    assert(len(dictionaries) == factors + 1) # one dictionary per source factor + 1 for target factor
    assert(len(model_options['dim_per_factor']) == factors) # each factor embedding has its own dimensionality
    assert(sum(model_options['dim_per_factor']) == model_options['dim_word']) # dimensionality of factor embeddings sums up to total dimensionality of input embedding vector

    # load dictionaries and invert them
    worddicts = [None] * len(dictionaries)
    worddicts_r = [None] * len(dictionaries)
    for ii, dd in enumerate(dictionaries):
        worddicts[ii] = load_dict(dd)
        worddicts_r[ii] = dict()
        for kk, vv in worddicts[ii].iteritems():
            worddicts_r[ii][vv] = kk

    # reload options
    if reload_ and os.path.exists(saveto):
        print 'Reloading model options'
        try:
            with open('%s.json' % saveto, 'rb') as f:
                model_options = json.load(f)
        except:
            with open('%s.pkl' % saveto, 'rb') as f:
                model_options = pkl.load(f)

    print 'Loading data'
    train = TextIterator(datasets[0], datasets[1],
                         dictionaries[:-1], dictionaries[-1],
                         n_words_source=n_words_src, n_words_target=n_words,
                         batch_size=batch_size,
                         maxlen=maxlen,
                         shuffle_each_epoch=shuffle_each_epoch,
                         maxibatch_size=maxibatch_size)
    valid = TextIterator(valid_datasets[0], valid_datasets[1],
                         dictionaries[:-1], dictionaries[-1],
                         n_words_source=n_words_src, n_words_target=n_words,
                         batch_size=valid_batch_size,
                         maxlen=maxlen)

    print 'Building model'
    params = init_params(model_options)
    # reload parameters
    if reload_ and os.path.exists(saveto):
        print 'Reloading model parameters'
        params = load_params(saveto, params)

    tparams = init_tparams(params)

    trng, use_noise, \
        x, x_mask, y, y_mask, \
        opt_ret, \
        cost = \
        build_model(tparams, model_options)

    inps = [x, x_mask, y, y_mask]

    print 'Building sampler'
    f_init, f_next = build_sampler(tparams, model_options, use_noise, trng)

    # before any regularizer
    print 'Building f_log_probs...',
    f_log_probs = theano.function(inps, cost, profile=profile)
    print 'Done'

    cost = cost.mean()

    # apply L2 regularization on weights
    if decay_c > 0.:
        decay_c = theano.shared(numpy.float32(decay_c), name='decay_c')
        weight_decay = 0.
        for kk, vv in tparams.iteritems():
            weight_decay += (vv ** 2).sum()
        weight_decay *= decay_c
        cost += weight_decay

    # regularize the alpha weights
    if alpha_c > 0. and not model_options['decoder'].endswith('simple'):
        alpha_c = theano.shared(numpy.float32(alpha_c), name='alpha_c')
        alpha_reg = alpha_c * (
            (tensor.cast(y_mask.sum(0)//x_mask.sum(0), 'float32')[:, None] -
             opt_ret['dec_alphas'].sum(0))**2).sum(1).mean()
        cost += alpha_reg

    # after all regularizers - compile the computational graph for cost
    print 'Building f_cost...',
    f_cost = theano.function(inps, cost, profile=profile)
    print 'Done'

    # allow finetuning with fixed embeddings
    if finetune:
        updated_params = OrderedDict([(key,value) for (key,value) in tparams.iteritems() if not key.startswith('Wemb')])
    else:
        updated_params = tparams

    print 'Computing gradient...',
    grads = tensor.grad(cost, wrt=itemlist(updated_params))
    print 'Done'

    # apply gradient clipping here
    if clip_c > 0.:
        g2 = 0.
        for g in grads:
            g2 += (g**2).sum()
        new_grads = []
        for g in grads:
            new_grads.append(tensor.switch(g2 > (clip_c**2),
                                           g / tensor.sqrt(g2) * clip_c,
                                           g))
        grads = new_grads

    # compile the optimizer, the actual computational graph is compiled here
    lr = tensor.scalar(name='lr')

    print 'Building optimizers...',
    f_grad_shared, f_update = eval(optimizer)(lr, updated_params, grads, inps, cost)
    print 'Done'

    print 'Optimization'

    best_p = None
    bad_counter = 0
    uidx = 0
    estop = False
    history_errs = []
    # reload history
    if reload_ and os.path.exists(saveto):
        rmodel = numpy.load(saveto)
        history_errs = list(rmodel['history_errs'])
        if 'uidx' in rmodel:
            uidx = rmodel['uidx']

    if validFreq == -1:
        validFreq = len(train[0])/batch_size
    if saveFreq == -1:
        saveFreq = len(train[0])/batch_size
    if sampleFreq == -1:
        sampleFreq = len(train[0])/batch_size

    for eidx in xrange(max_epochs):
        n_samples = 0

        for x, y in train:
            n_samples += len(x)
            uidx += 1
            use_noise.set_value(1.)

            #ensure consistency in number of factors
            if len(x) and len(x[0]) and len(x[0][0]) != factors:
                sys.stderr.write('Error: mismatch between number of factors in settings ({0}), and number in training corpus ({1})\n'.format(factors, len(x[0][0])))
                sys.exit(1)

            x, x_mask, y, y_mask = prepare_data(x, y, maxlen=maxlen,
                                                n_words_src=n_words_src,
                                                n_words=n_words)

            if x is None:
                print 'Minibatch with zero sample under length ', maxlen
                uidx -= 1
                continue

            ud_start = time.time()

            # compute cost, grads and copy grads to shared variables
            cost = f_grad_shared(x, x_mask, y, y_mask)

            # do the update on parameters
            f_update(lrate)

            ud = time.time() - ud_start

            # check for bad numbers, usually we remove non-finite elements
            # and continue training - but not done here
            if numpy.isnan(cost) or numpy.isinf(cost):
                print 'NaN detected'
                return 1., 1., 1.

            # verbose
            if numpy.mod(uidx, dispFreq) == 0:
                print 'Epoch ', eidx, 'Update ', uidx, 'Cost ', cost, 'UD ', ud

            # save the best model so far, in addition, save the latest model
            # into a separate file with the iteration number for external eval
            if numpy.mod(uidx, saveFreq) == 0:
                print 'Saving the best model...',
                if best_p is not None:
                    params = best_p
                else:
                    params = unzip(tparams)
                numpy.savez(saveto, history_errs=history_errs, uidx=uidx, **params)
                json.dump(model_options, open('%s.json' % saveto, 'wb'), indent=2)
                print 'Done'

                # save with uidx
                if not overwrite:
                    print 'Saving the model at iteration {}...'.format(uidx),
                    saveto_uidx = '{}.iter{}.npz'.format(
                        os.path.splitext(saveto)[0], uidx)
                    numpy.savez(saveto_uidx, history_errs=history_errs,
                                uidx=uidx, **unzip(tparams))
                    print 'Done'


            # generate some samples with the model and display them
            if numpy.mod(uidx, sampleFreq) == 0:
                # FIXME: random selection?
                for jj in xrange(numpy.minimum(5, x.shape[2])):
                    stochastic = True
                    sample, score, sample_word_probs, alignment = gen_sample([f_init], [f_next],
                                               x[:, :, jj][:, :, None],
                                               trng=trng, k=1,
                                               maxlen=30,
                                               stochastic=stochastic,
                                               argmax=False,
                                               suppress_unk=False)
                    print 'Source ', jj, ': ',
                    for pos in range(x.shape[1]):
                        if x[0, pos, jj] == 0:
                            break
                        for factor in range(factors):
                            vv = x[factor, pos, jj]
                            if vv in worddicts_r[factor]:
                                sys.stdout.write(worddicts_r[factor][vv])
                            else:
                                sys.stdout.write('UNK')
                            if factor+1 < factors:
                                sys.stdout.write('|')
                            else:
                                sys.stdout.write(' ')
                    print
                    print 'Truth ', jj, ' : ',
                    for vv in y[:, jj]:
                        if vv == 0:
                            break
                        if vv in worddicts_r[-1]:
                            print worddicts_r[-1][vv],
                        else:
                            print 'UNK',
                    print
                    print 'Sample ', jj, ': ',
                    if stochastic:
                        ss = sample
                    else:
                        score = score / numpy.array([len(s) for s in sample])
                        ss = sample[score.argmin()]
                    for vv in ss:
                        if vv == 0:
                            break
                        if vv in worddicts_r[-1]:
                            print worddicts_r[-1][vv],
                        else:
                            print 'UNK',
                    print

            # validate model on validation set and early stop if necessary
            if numpy.mod(uidx, validFreq) == 0:
                use_noise.set_value(0.)
                valid_errs, alignment = pred_probs(f_log_probs, prepare_data,
                                        model_options, valid)
                valid_err = valid_errs.mean()
                history_errs.append(valid_err)

                if uidx == 0 or valid_err <= numpy.array(history_errs).min():
                    best_p = unzip(tparams)
                    bad_counter = 0
                if len(history_errs) > patience and valid_err >= \
                        numpy.array(history_errs)[:-patience].min():
                    bad_counter += 1
                    if bad_counter > patience:
                        print 'Early Stop!'
                        estop = True
                        break

                if numpy.isnan(valid_err):
                    ipdb.set_trace()

                print 'Valid ', valid_err

                if external_validation_script:
                    print "Calling external validation script"
                    print 'Saving  model...',
                    params = unzip(tparams)
                    numpy.savez(saveto +'.dev', history_errs=history_errs, uidx=uidx, **params)
                    json.dump(model_options, open('%s.dev.npz.json' % saveto, 'wb'), indent=2)
                    print 'Done'
                    p = Popen([external_validation_script])

            # finish after this many updates
            if uidx >= finish_after:
                print 'Finishing after %d iterations!' % uidx
                estop = True
                break

        print 'Seen %d samples' % n_samples

        if estop:
            break

    if best_p is not None:
        zipp(best_p, tparams)

    use_noise.set_value(0.)
    valid_errs, alignment = pred_probs(f_log_probs, prepare_data,
                                       model_options, valid)
    valid_err = valid_errs.mean()

    print 'Valid ', valid_err

    params = copy.copy(best_p)
    numpy.savez(saveto, zipped_params=best_p,
                history_errs=history_errs,
                uidx=uidx,
                **params)

    return valid_err


if __name__ == '__main__':
    pass
