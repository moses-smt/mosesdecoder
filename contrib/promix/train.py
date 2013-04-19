#!/usr/bin/env python

#
# Train the model weights
#

from math import log,exp
import sys
import numpy as np
from scipy.optimize.optimize import fmin_cg, fmin_bfgs, fmin
from scipy.optimize.lbfgsb import fmin_l_bfgs_b

import nbest
from util import safelog

def sigmoid(x):
  return 1.0 / (1.0 + np.exp(-x))

class OptimisationException(Exception):
  pass

class ParabaloidOptimiser:
  """Optimises a very simple function, to test scipy"""
  def __init__(self, params):
    self.params = params

  def objective(self,x):
    return np.sum(x*x*self.params*self.params)

  def grad_k(self,x,k):
    return 2 * self.params[k]**2 * x[k]

  def grad(self,x):
    return np.array([self.grad_k(x,k) for k in range(len(x))])

  def debug(self,x):
    print "x = ",x


  def optimise_bfgs(self,start):
    print
    print "***** BFGS OPTIMISATION *****"
    return fmin_bfgs(self.objective, start, fprime=self.grad, callback=self.debug)

  def optimise_lbfgs(self,start):
    print
    print "***** LBFGS OPTIMISATION  *****"
    x,f,d = fmin_l_bfgs_b(self.objective, start, fprime=self.grad, pgtol=1e-09, iprint=0)
    return x

class LRDataException(Exception):
  pass
        
class LogisticRegressionOptimiser:
  """Optimise logistic regression weights"""
  def __init__(self,x,y, alpha = 0):
    """Training data (x) should be vector of feature vectors, and 
    corresponding vector of outputs (with values -1,1).
    alpha controls the L2-normalisation"""
    self.x = x
    self.y = y
    self.alpha = alpha
    if len(x) != len(y): raise LRDataException("Lengths of input and response don't match") 
    if len(x) == 0: raise LRDataException("Data set is empty")
    # Precalculate {y_i*x_ij} for all j
    self.xy = x*y[:,None]

  def objective(self,w):
    """Calculate the value of the negative log-likelihood for a given weight set"""
    l = 0
    for i in range(len(self.x)):
      # Each example contributes log(sigma(y_i * x_i . w))
      l -= log(sigmoid(self.y[i] * np.dot(w, self.x[i,:])))
    # regularisation 1/2 * alpha * ||w||^2
    l += 0.5 * self.alpha * np.dot(w,w)
    return l

  def grad_j(self,w,j):
    """Gradient of the objective in the jth direction for given weight set"""
    g = 0
    for i in range(len(self.x)):
      # Each example contributes -sigma(-y_i * x_i.w) * y_j x_ij
      g -= sigmoid(-self.y[i] * np.dot(w, self.x[i,:])) * self.y[i] * self.x[i,j]
    #regularisation
    g += self.alpha * w[j]
    return g 

  def grad(self,w):
    """Gradient of objective at given weight set - returns a vector"""
    # Calculate the vector -sigma(-y_i * x_i.w)
    s = -np.array([sigmoid(-yi * np.dot(xi,w)) for xi,yi in zip(self.x,self.y)])
    # Multiply it by xy
    g = np.array([np.dot(xyj,s) for xyj in self.xy.transpose()])
    # Add regularisation
    g += self.alpha*w
    return g
    #g = np.array([self.grad_j(w,j) for j in xrange(len(w))])


  def train(self,w0,debug=False):
    if debug:
      iprint = 0
    else:
      iprint = -1
    x,f,d = fmin_l_bfgs_b(self.objective, w0, fprime=self.grad, pgtol=1e-09, iprint=iprint)
    if d['warnflag'] != 0:
      raise OptimisationException(d['task'])
    return x
    
class ProTrainer:
  """Turns the samples into a logistic regression problem"""
  def __init__(self,samples):
    self.samples = samples
    self.alpha = 1
    self.dims = len(samples[0].hyp1.fv)


  def train(self, debug=False):
    x = np.array([s.hyp1.fv-s.hyp2.fv for s in self.samples])
    #print x
    y = np.array([cmp(s.hyp1.score,s.hyp2.score) for s in self.samples])
    #print y
    lro = LogisticRegressionOptimiser(x,y,self.alpha)
    w0 = np.zeros(self.dims)
    w = lro.train(w0,debug)
    w = w/np.sum(abs(w)) # L_1 normalise
    return w,[]

class MixtureModelTrainer:
  """Trains the phrase mixture weights, as well as the regular feature weights"""
  def __init__(self,samples):
    self.alpha = 1
    self.interp_floor = 0.001 # minimum value for interpolation weight
    #self.prob_floor = 0.00000001 # floor probabilities at this value 
    #self.weight_bounds = (-10,10) # bounds for other features
    # The phrase scores are joined into a 5d array, where the dimensions are:
    # sample, hyp1 or hyp2, ttable, phrase-pair, feature
    # ie the feature is the last dimension
    # Actually phrase_probs is a 2-dim list of 3-dim arrays, since it's ragged
    self.phrase_probs = \
      [[sample.hyp1.phrase_scores,sample.hyp2.phrase_scores]\
        for sample in samples]
      #[[sample.hyp1.phrase_scores.clip(self.prob_floor),sample.hyp2.phrase_scores.clip(self.prob_floor)]\

    # Figure out where the weights are
    self.phrase_index = list(nbest.get_feature_index("tm"))
    self.phrase_index[1] = self.phrase_index[1]-1 # phrase penalty not interpolated

    interp_length = (self.phrase_index[1]-self.phrase_index[0]) * \
      (len(samples[0].hyp1.phrase_scores)-1)
    weight_length = len(samples[0].hyp1.fv) + interp_length
    self.interp_index = [weight_length - interp_length,weight_length]
    #print self.interp_index
    self.other_index = [[0,self.phrase_index[0]],[self.phrase_index[1],self.interp_index[0]]]

    # join the feature vector diffs for the other fvs into a 2d array
    # features across, samples down
    self.fvs = np.array(\
      [np.append(sample.hyp1.fv[self.other_index[0][0]:self.other_index[0][1]],
           sample.hyp1.fv[self.other_index[1][0]:self.other_index[1][1]]) - \
        np.append(sample.hyp2.fv[self.other_index[0][0]:self.other_index[0][1]],\
           sample.hyp2.fv[self.other_index[1][0]:self.other_index[1][1]])\
          for sample in samples])
    
    self.cached_iw = None
    self.cached_interpolated_phrase_probs = None

    self.cached_sw = None
    self.cached_y_times_diffs = None



    # join the responses (y's) into an array
    # If any pairs have equal score, this sets y=0, an invalid response.
    # but the sampling should ensure that this doesn't happen
    self.y = np.array([cmp(sample.hyp1.score, sample.hyp2.score)\
      for sample in samples])

  def get_split_weights(self,weights):
    """Map containing all the different weight sets:
       phrase - phrase feature weights (excluding penalty)
       other - other feature weights
       interp - interpolation weights: ttable x feature
       """
    sw = {}
    sw['phrase'] = weights[self.phrase_index[0]:self.phrase_index[1]]
    sw['interp'] = weights[self.interp_index[0]:self.interp_index[1]]
    sw['interp'] = sw['interp'].T.reshape\
      (( len(sw['interp']) / len(sw['phrase'])), len(sw['phrase']))
    # Add normalisations
    sw['interp'] = np.vstack((sw['interp'], 1.0 - np.sum(sw['interp'], axis=0))) 
    #sw['interp'] = np.append(sw['interp'], 1 - np.sum(sw['interp']))
    sw['other'] = np.append(weights[self.other_index[0][0]:self.other_index[0][1]],
                            weights[self.other_index[1][0]:self.other_index[1][1]])
    return sw

  def get_interpolated_phrase_probs(self,iw):
    # Memoise
    if self.cached_iw == None or np.sum(np.abs(iw-self.cached_iw)) != 0:
      # iw is ttable x feature. Each element of phrase_probs is ttable x pair x feature
      iw_expanded = np.expand_dims(iw,1)
      # self.phrase probs is a 2-d list, so use python iteration
      interpolated = [ [iw_expanded*p for p in ps] for ps in self.phrase_probs]
      self.cached_interpolated_phrase_probs = np.sum(np.array(interpolated), axis = 2)
      self.cached_iw = iw
    return self.cached_interpolated_phrase_probs

  def get_y_times_diffs(self,sw):
    """ Calculate the array y_k* \Delta S_k"""
    # Process the phrase scores first.
    # - for each phrase, interpolate across the ttables using the current weights
    # - sum the log probs across phrase pairs to get a score for each hypothesis
    # - take the weighted sum of these scores, to give a phrase feature total
    #       for each hyp

    # Memoise
    if self.cached_sw == None or \
      np.sum(np.abs(self.cached_sw['other'] - sw['other'])) != 0 or \
      np.sum(np.abs(self.cached_sw['phrase'] - sw['phrase'])) != 0 or \
      np.sum(np.abs(self.cached_sw['interp'] - sw['interp'])) != 0:

      # do the interpolation
      iw = sw['interp']
      interpolated = self.get_interpolated_phrase_probs(iw)
      # Use traditional python as not sure how to vectorise. This goes through
      # each hypothesis, logs the probability, applies the feature weights, then sums
      self.cached_y_times_diffs = np.zeros(len(interpolated))
      # Take the difference between the hypotheses
      for i,sample in enumerate(interpolated):
        self.cached_y_times_diffs[i] = \
          np.sum(sw['phrase']* np.log(sample[0])) - \
          np.sum(sw['phrase']* np.log(sample[1]))
      #print self.fvs, sw['other']
      #print sw['other'], self.fvs
      self.cached_y_times_diffs += np.sum(sw['other'] * self.fvs, axis=1) # add other scores
      self.cached_y_times_diffs *= self.y
      self.cached_sw = sw
    return self.cached_y_times_diffs

  def objective(self,w):
    """The value of the objective with the given weight vector.
       The objective is the sum of the log of the sigmoid of the  differences
       in scores between the two hypotheses times y.
     """
    diffs = self.get_y_times_diffs(self.get_split_weights(w))
    #print diffs, sigmoid(diffs)
    obj = -np.sum(np.log(sigmoid(diffs))) #negative, since minimising
    # regularisation
    obj += 0.5 * self.alpha * np.dot(w[:self.interp_index[0]], w[:self.interp_index[0]])
    return obj

  #
  # The following methods compute the derivatives of the score differences
  # with respect to each of the three types of weights. They should all 
  # return an np.array, with features across, and samples down
  #

  def gradient_phrase(self,interp):
    """Compute the derivative of the score difference for the 'phrase' weights.

    Args:
      interp: The interpolation weights
      """
    #  Compute the interpolated phrase probs
    interpolated = self.get_interpolated_phrase_probs(interp)

    # for each sample, log and sum across phrases, then compute the feature value
    # difference  for each sample.
    # TODO: Better vectorisation
    grad_list = []
    for i, sample in enumerate(interpolated):
      f_A = np.sum(np.log(sample[0]), axis=0)
      f_B = np.sum(np.log(sample[1]), axis=0) 
      grad_list.append(f_A - f_B)
    return np.vstack(grad_list)

  def gradient_interp(self,interp,phrase):
    """Compute the derivative of the score difference for the 'interp' weights

    Args:
      interp: All the interpolation weights. These will be in a 2-dim np array,
          where the dims are ttable x phrase feature. Note that there are k rows,
          one for each ttable, so the sum down the columns will be 1.
      phrase: The weights of the phrase features

    Returns:
      A 2-d array, with samples down and gradients along. Note that in the gradients
      (rows) the interpolation weights are flattened out, and have the last ttable 
      removed.
    """
    num_interp_weights = (interp.shape[0]-1) * interp.shape[1]
    grad_list = np.empty((len(self.phrase_probs),num_interp_weights))
    expanded_interp = np.expand_dims(interp,1)
    def df_by_dlambda(phi):
      """Derivative of phrase scores w.r.t. lambdas"""
      #print "Interp:", interp, "\nPhi", phi
      num = phi[:-1] - phi[-1]
      denom = np.sum(expanded_interp*phi, axis=0)
      # num is ttable x phrase-pair x feature
      # denom is phrase-pair x feature
      # divide, then sum across phrase-pairs
      #print "num",num,"denom",denom
      #print "q",num/denom
      quotient = np.sum(num/denom, axis =1)
      # quotient is ttable-1 x feature
      return quotient


    for k, sample in enumerate(self.phrase_probs):
      # derivative is the weighted sum of df_by_dlambda_A - df_by_dlambda_B
      #print "\nq0", df_by_dlambda(sample[0])
      #print "hyp0",np.sum(phrase * (df_by_dlambda(sample[0])), axis=0)
      #print "q1", df_by_dlambda(sample[1])
      #print "hyp1",np.sum(phrase * (df_by_dlambda(sample[1])), axis=0),"\n"
      #TODO: Check if the sum is required here. With 4 ttables and 4 features
      # it gives lhs as (12) and rhs as (4)
      grad_list[k] = (phrase * (df_by_dlambda(sample[0]) - df_by_dlambda(sample[1]))).flatten()
    #grad_list = np.vstack(grad_list)
    return grad_list

  def gradient_other(self):
    """Compute the derivative of the score difference for the 'other' weights.

      Features across, samples down.
        """
    # This is just the difference in the feature values
    return self.fvs

  def gradient(self,w):
    sw = self.get_split_weights(w)
    sig_y_by_diffs = sigmoid(-self.get_y_times_diffs(sw))
    # These all return 2-d arrays, with samples dowm and features across.
    # NB: Both gradient_phrase and gradient_interp iterate through the samples,
    #     so this is probably inefficient
    phrase_g = self.gradient_phrase(sw['interp'])
    interp_g = self.gradient_interp(sw['interp'], sw['phrase'])
    other_g = self.gradient_other()

    # For each feature, we get the gradient by multiplying by \sigma (-y*\Delta S),
    # multiplying by y, and summing across all samples
    # Take negatives since we're minimising
    phrase_g = -np.sum(np.transpose(phrase_g) * sig_y_by_diffs * self.y, axis=1)
    interp_g = -np.sum(np.transpose(interp_g) * sig_y_by_diffs * self.y, axis=1)
    other_g = -np.sum(np.transpose(other_g) * sig_y_by_diffs * self.y, axis=1)

    # regularisation
    phrase_g += self.alpha * sw['phrase']
    other_g += self.alpha * sw['other']

    # Splice the gradients together
    grad = np.array([0.0]* len(w))
    grad[-len(interp_g):] = interp_g
    grad[self.phrase_index[0]:self.phrase_index[1]] = phrase_g
    grad[self.other_index[0][0]:self.other_index[0][1]] = \
      other_g[:self.other_index[0][1] - self.other_index[0][0]]
    grad[self.other_index[1][0]:self.other_index[1][1]] = \
      other_g[self.other_index[0][1] - self.other_index[0][0]:]
    return grad


  def train(self,debug=False):
    """Train the mixture model."""
    if debug:
      iprint = 0
    else:
      iprint = -1
    # Initialise weights to zero, except interpolation
    num_phrase_features = self.phrase_index[1] - self.phrase_index[0]
    num_models = ((self.interp_index[1] - self.interp_index[0])/num_phrase_features)+1
    w0 = [0.0] * self.interp_index[0]
    w0 += [1.0/num_models] * (self.interp_index[1]-self.interp_index[0])
    bounds = [(None,None)] * len(w0)
    bounds[self.interp_index[0]:self.interp_index[1]] = \
      [(self.interp_floor,1)] * (self.interp_index[1] - self.interp_index[0])
    w0 = np.array(w0)
    x,f,d = fmin_l_bfgs_b(self.objective, w0, fprime=self.gradient, bounds=bounds,  pgtol=1e-09, iprint=iprint)
    if d['warnflag'] != 0:
      raise OptimisationException(d['task'])
    weights = x[:self.interp_index[0]]
    mix_weights = x[self.interp_index[0]:]
    mix_weights = mix_weights.reshape((num_models-1,num_phrase_features))
    mix_weights = np.vstack((mix_weights, 1-np.sum(mix_weights,axis=0)))
    return weights,mix_weights
    

#
# Test logistic regression using pro data
#
def main():
  fh = open("data/esen.wmt12.pro")
  x = []
  y = []
  d = 14
  for line in fh:
    line = line[:-1]
    fields = line.split()
    if fields[0] == "1":
      y.append(1)
    else:
      y.append(-1)
    x_i = [0]*d
    for i in xrange(1,len(fields),2):
        j = int(fields[i][1:])
        x_ij = float(fields[i+1])
        x_i[j] = x_ij
    x.append(x_i)
  lro = LogisticRegressionOptimiser(np.array(x), np.array(y), 0.1)
  print lro.train(np.zeros(d), debug=True)


if __name__ == "__main__":
  main()
