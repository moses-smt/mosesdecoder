#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import sys
import numpy as np


def logit(X):
    return 1.0 / (1.0 + np.exp(-X))


def softmax(X, ax=0):
    expX = np.exp(X)
    expXsum = np.sum(expX, axis=ax)
    return (expX / expXsum)


def slice(x, n, dim):
    if x.ndim == 3:
        return x[:, :, n*dim:(n+1)*dim]
    if x.ndim == 1:
        return x[n*dim:(n+1)*dim]
    return x[:, n*dim:(n+1)*dim]


class Encoder(object):
    class Embeddings:
        def __init__(self, model):
            self.E = model['Wemb']

        def Lookup(self, index):
            return self.E[index]

    class RNN(object):
        def __init__(self, model):
            self.W = model['encoder_W']
            self.U = model['encoder_U']
            self.b = model['encoder_b']

            self.Wx = model['encoder_Wx']
            self.Ux = model['encoder_Ux']
            self.bx = model['encoder_bx']

        def GetContext(self, srcEmbs):
            context = []
            prevState = self.InitState()
            for emb in srcEmbs:
                state = self.GenNextState(emb, prevState)
                context.append(state)
                prevState = state
            return context

        def InitState(self):
            return np.zeros(1024).reshape(1, 1024)

        def GenNextState(self, emb, prevState):
            ru = logit(np.dot(emb, self.W)
                       + self.b
                       + np.dot(prevState, self.U))

            r = slice(ru, 0, 1024)
            u = slice(ru, 1, 1024)

            quasiState = np.tanh(np.dot(emb, self.Wx)
                                 + self.bx
                                 + r * np.dot(prevState, self.Ux))

            newState = (1.0 - u) * quasiState + u * prevState

            return newState

    def __init__(self, model):
        fw = dict()
        fw['encoder_W'] = model['encoder_W']
        fw['encoder_U'] = model['encoder_U']
        fw['encoder_b'] = model['encoder_b']

        fw['encoder_Wx'] = model['encoder_Wx']
        fw['encoder_Ux'] = model['encoder_Ux']
        fw['encoder_bx'] = model['encoder_bx']

        bw = dict()
        bw['encoder_W'] = model['encoder_r_W']
        bw['encoder_U'] = model['encoder_r_U']
        bw['encoder_b'] = model['encoder_r_b']

        bw['encoder_Wx'] = model['encoder_r_Wx']
        bw['encoder_Ux'] = model['encoder_r_Ux']
        bw['encoder_bx'] = model['encoder_r_bx']

        self.fRNN = self.RNN(fw)
        self.bRNN = self.RNN(bw)
        self.Emb = self.Embeddings(model)

    def GetContext(self, srcSentece):
        embs = [self.Emb.Lookup(i) for i in srcSentece]
        fContext = np.concatenate(self.fRNN.GetContext(embs))
        bContext = np.concatenate(self.bRNN.GetContext(embs[::-1])[::-1])
        cc = np.hstack([fContext, bContext])
        return cc


class Decoder(object):
    class Embeddings:
        def __init__(self, model):
            self.E = model['Wemb_dec']

        def Init(self):
            return np.zeros(500)

        def Lookup(self, index):
            return self.E[index]

    class RNN(object):
        def __init__(self, model):
            self.WI = model['ff_state_W']
            self.bI = model['ff_state_b']

            self.U = model['decoder_U']
            self.W = model['decoder_W']
            self.b = model['decoder_b']
            self.Ux = model['decoder_Ux']
            self.Wx = model['decoder_Wx']
            self.bx = model['decoder_bx']

            self.Up = model['decoder_U_nl']
            self.Wp = model['decoder_Wc']
            self.bp = model['decoder_b_nl']
            self.Upx = model['decoder_Ux_nl']
            self.Wpx = model['decoder_Wcx']
            self.bpx = model['decoder_bx_nl']

        def InitState(self, context):
            cc = np.mean(context, axis=0)
            print cc
            return np.tanh(np.dot(cc, self.WI) + self.bI)

        def GenMiddleState(self, state, emb):
            ru = logit(np.dot(state, self.U) + np.dot(emb, self.W) + self.b)
            r = slice(ru, 0, 1024)
            u = slice(ru, 1, 1024)
            s_m = np.tanh(np.dot(state, self.Ux) * r
                          + np.dot(emb, self.Wx)
                          + self.bx)
            new_state = u * state + (1.0 - u) * s_m
            return new_state

        def GenNewState(self, s_m, cc):
            ru = logit(np.dot(s_m, self.Up) + self.bp + np.dot(cc, self.Wp))
            r = slice(ru, 0, 1024)
            u = slice(ru, 1, 1024)
            s_ = np.tanh((np.dot(s_m, self.Upx) + self.bpx) * r
                         + np.dot(cc, self.Wpx))
            s = u * s_m + (1.0 - u) * s_
            return s

    class AttentionModel(object):
        def __init__(self, model):
            self.W = model['decoder_W_comb_att']
            self.b = model['decoder_b_att']
            self.U = model['decoder_Wc_att']
            self.V = model['decoder_U_att']
            self.c = model['decoder_c_tt']

        def GetAttention(self, state, context):
            e = np.tanh(np.dot(state, self.W)
                        + self.b
                        + np.dot(context, self.U))
            e = np.dot(e, self.V) + self.c
            
            alpha = softmax(e)
            cc = np.sum(context * alpha, axis=0)
            return cc

    class ReadOut(object):
        def __init__(self, model):
            self.W1 = model['ff_logit_lstm_W']
            self.b1 = model['ff_logit_lstm_b']

            self.W2 = model['ff_logit_prev_W']
            self.b2 = model['ff_logit_prev_b']

            self.W3 = model['ff_logit_ctx_W']
            self.b3 = model['ff_logit_ctx_b']

            self.W4 = model['ff_logit_W']
            self.b4 = model['ff_logit_b']

        def GetProbs(self, state, ctx, prevEmb):
            t = np.tanh(np.dot(state, self.W1) + self.b1
                        + np.dot(prevEmb, self.W2) + self.b2
                        + np.dot(ctx, self.W3) + self.b3)
            return softmax(np.dot(t, self.W4) + self.b4)

    def __init__(self, model):
        self.emb = self.Embeddings(model)
        self.readOut = self.ReadOut(model)
        self.rnn = self.RNN(model)
        self.att = self.AttentionModel(model)

    def ScoreSentence(self, sentence, context):
        s_prev = self.rnn.InitState(context)
        print s_prev.shape
        print s_prev
        score = 0.0
        emb = np.zeros(500)
        for word in sentence:
            s_m = self.rnn.GenMiddleState(s_prev, emb)
            cc = self.att.GetAttention(s_m, context)
            s = self.rnn.GenNewState(s_m, cc)
            print "State: "
            print s
            probs = self.readOut.GetProbs(s, cc, emb)
            lprob = np.log(probs[word])
            print lprob
            score += lprob
            emb = self.emb.Lookup(word)
            s_prev = s
        return score


def main():
    modelPath = sys.argv[1]
    model = np.load(modelPath)
    encoder = Encoder(model)
    decoder = Decoder(model)

    cc = encoder.GetContext(np.array([307, 24, 5, 0]))
    print decoder.ScoreSentence([256, 465, 4, 0], cc)


if __name__ == "__main__":
    main()
