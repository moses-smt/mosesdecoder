#!/usr/bin/env python

import unittest

from math import log,exp

import nbest
import numpy.testing as nptest
import sampler
from train import *
import util

class TestParabaloidOptimiser(unittest.TestCase):
  def setUp(self):
    self.o = ParabaloidOptimiser(np.array([1,2,3,4]))

  def test_parabaloid_bfgs(self):
    start = np.array([2,2,2,2])
    minimum = self.o.optimise_bfgs(start)
    for m in minimum:
      self.assertAlmostEqual(m,0)


  def test_parabaloid_lbfgs(self):
    start = np.array([2,2,2,2])
    minimum = self.o.optimise_lbfgs(start)
    for m in minimum:
      self.assertAlmostEqual(m,0)

class TestLogisticRegressionOptimiser(unittest.TestCase):

  def test_objective(self):
    x = np.array([[1], [0]])
    y = np.array([1,-1])
    lro = LogisticRegressionOptimiser(x,y)
    w = np.array([2])
    expected = -log(1 / (1 + exp(-2))) - log(0.5)
    self.assertAlmostEqual(lro.objective(w), expected)
    
  def test_reg_objective(self):
    x = np.array([[1], [0]])
    y = np.array([1,-1])
    alpha = 0.1
    lro = LogisticRegressionOptimiser(x,y,alpha)
    w = np.array([2])
    expected = -log(1 / (1 + exp(-2))) - log(0.5) + 0.5*2*2 * alpha
    self.assertAlmostEqual(lro.objective(w), expected)
  
  def test_gradient_j(self):
    x = np.array([[1], [0]])
    y = np.array([1,-1])
    lro = LogisticRegressionOptimiser(x,y)
    w = np.array([2])
    expected = -1 / (1 + exp(2))
    self.assertAlmostEqual(lro.grad_j(w,0), expected)

  def test_gradient(self):
    x = np.array([[1,1], [0,1]])
    y = np.array([1,-1])
    w = np.array([2,1])
    lro = LogisticRegressionOptimiser(x,y)
    e0 = -1 / (1 + exp(3))
    e1 = -1 / (1 + exp(3)) + 1/ (1 + exp(-1))
    actual = lro.grad(w)
    #print "expected: ",e0,e1
    self.assertAlmostEqual(actual[0], e0)
    self.assertAlmostEqual(actual[1], e1)

  def test_reg_gradient(self):
    x = np.array([[1,1], [0,1]])
    y = np.array([1,-1])
    alpha = 0.2
    w = np.array([2,1])
    lro = LogisticRegressionOptimiser(x,y, alpha)
    e0 = -1 / (1 + exp(3)) + w[0]*alpha
    e1 = -1 / (1 + exp(3)) + 1/ (1 + exp(-1)) +w[1]*alpha
    actual = lro.grad(w)
    self.assertAlmostEqual(actual[0], e0)
    self.assertAlmostEqual(actual[1], e1)
    

  def test_train(self):
    x = np.array([[1,1],[-1,-2]])
    y = np.array([1,-1])
    w0 = np.array([1,-1])
    lro = LogisticRegressionOptimiser(x,y)
    actual = lro.train(w0, debug=False)
    self.assertAlmostEqual(actual[0], 12.03882542)
    self.assertAlmostEqual(actual[1], 8.02317419)

  def test_train_reg(self):
    x = np.array([[1,1],[-1,1]])
    y = np.array([1,-1])
    alpha = 0.1
    w0 = np.array([1,-1])
    lro = LogisticRegressionOptimiser(x,y,alpha)
    actual = lro.train(w0, debug=False)
    self.assertAlmostEqual(actual[1],0) # 2nd input should be ignored
    # classify first example as negative, second as positive
    self.assertTrue(1 / (1+exp(-np.dot(actual,np.array([1,1])))) > 0.5)
    self.assertTrue(1 / (1+exp(-np.dot(actual,np.array([-1,-2])))) <  0.5)

  def test_xy(self):
    """Test pre-calculation of the y_i*x_ij vectors"""
    x = np.array([[1,3], [2,8], [1,3]])
    y = np.array([1,1,-1])
    lro = LogisticRegressionOptimiser(x,y)
    expected = np.array([[1,3], [2,8], [-1,-3]])
    for i in 0,1,2:
      for j in 0,1:
        self.assertEqual(lro.xy[i][j], expected[i][j])
#
class TestMixtureModelTrainer(unittest.TestCase):
  
  def setUp(self):
    # 3 phrase table features, but last one is ignored for interpolation
    nbest._feature_index = {"tm" : [0,3], "lm" : [3,4]}
    log05 = np.log(0.5)
    log03 = np.log(0.3)
    log02 = np.log(0.2)
    log01 = np.log(0.1)
    hyp0 = nbest.Hypothesis("a |0-0| b c |1-2|", [log05, log05, log02, log03], True)
    hyp0.input_line = "A B C"
    hyp0.score = 3
    # Two ttables, columns correspond to features, rows to phrase pairs
    hyp0.phrase_scores = np.array([\
      [[0.2, 0.3],\
       [0.4, 0.3]],\
      [[0, 0.2],\
       [0.4,0.2]]])

    hyp1 = nbest.Hypothesis("x |0-2|", [log02, log03, log03, log01], True)
    hyp1.input_line = "X Y Z"
    hyp1.score = 2
    hyp1.phrase_scores = np.array([\
      [[0.1, 0.1]],\
      [[0.8,0.1]]])

    hyp2 = nbest.Hypothesis("z |0-1| w |2-2| p |3-3|", [log02, log02, log05, log05], True)
    hyp2.score = 1
    hyp2.input_line = "M N O"
    # phrase_table x phrase_pair x feature
    hyp2.phrase_scores = np.array([\
      [[0.1, 0.2],\
       [0.3,0.5],\
       [0.4,0.6]],\
      [[0.1,0.5],\
       [0.6,0.1],\
       [0.2,0.2]]])
    self.samples = [sampler.Sample(hyp0,hyp1), sampler.Sample(hyp1,hyp2)]
    self.trainer = MixtureModelTrainer(self.samples)

  def get_phrase_scores(self, hypothesis, iw):
    nptest.assert_almost_equal(np.sum(iw, axis=0), np.array([1.0,1.0]))
    phrase_probs = hypothesis.phrase_scores 
    interpolated_probs = np.sum(np.expand_dims(iw,1)*phrase_probs, axis = 0)

    total_probs = np.prod(interpolated_probs, axis = 0)
    return util.safelog(total_probs)
 
  def model_score(self, hypothesis, weights):
    # interpolation weights
    # ttable x feature
    iw = np.array([[weights[-2], weights[-1]],
                    [1-weights[-2],1-weights[-1]]]) 
    #print "iw:",iw
    phrase_scores = self.get_phrase_scores(hypothesis,iw)
    weighted_phrase_scores = weights[:2] * phrase_scores
    score = np.sum(weighted_phrase_scores)

    other_score =  np.sum(weights[2:4]*hypothesis.fv[2:4])
    return score + other_score


  def test_objective(self):
    # 2 phrase weights, 2 other feature weights, 
    # 2 interpolation weights (1 per model x 2 phrase features)
    weights = np.array([0.2,0.1,0.4,0.5,0.3,0.6])
    actual = self.trainer.objective(weights)
    # Expected objective is the sum of the logs of  sigmoids of the score differences
    # Weighted by 1 if hyp1 > hyp2, -1 otherwise
    expected = 0
    for sample in self.samples:
      hyp1_model_score = self.model_score(sample.hyp1, weights)
      hyp2_model_score = self.model_score(sample.hyp2, weights)
      y = 1
      if sample.hyp2.score > sample.hyp1.score: y = -1
      expected -= log(sigmoid(y * (hyp1_model_score - hyp2_model_score)))
    # regularisation
    expected += 0.5 * self.trainer.alpha * np.dot(weights[:-2], weights[:-2])
    self.assertAlmostEquals(actual,expected)

  def test_gradient_other(self):
    # Gradients are just differences in feature vectors
    # fv(hypo0)-fv(hyp1), fv(hyp1)-fv(hyp2)
    delta_s = np.vstack((self.samples[0].hyp1.fv-self.samples[0].hyp2.fv,\
        self.samples[1].hyp1.fv-self.samples[1].hyp2.fv))
    # feature functions across rows, samples down columns
    # choose other features
    other_delta_s = delta_s[:,2:]
    actual = self.trainer.gradient_other()
    nptest.assert_almost_equal(actual,other_delta_s)

  def test_gradient_phrase(self):
    iw = np.array([[0.3, 0.4],[0.7,0.6]])
    sample_deltaf_list = []
    for sample in self.samples:
      f_A = self.get_phrase_scores(sample.hyp1, iw)
      f_B = self.get_phrase_scores(sample.hyp2, iw)
      sample_deltaf_list.append(f_A - f_B)
    expected = np.vstack(sample_deltaf_list) # samples down, features along
    actual = self.trainer.gradient_phrase(iw)
    nptest.assert_almost_equal(actual,expected)

  def test_gradient_interp(self):
    # The interpolation weights - ttable x feature
    iw = np.array([[0.3, 0.4],[0.7,0.6]]) 
    phrasew  = np.array([1,2]) # The phrase weights
    num_ttables = iw.shape[0]
    num_phrase_features = iw.shape[1]
    bysample_list = []
    # Stack up gradients for each sample
    for sample in self.samples:
      # Get the gradient of the interpolation weights for each
      # hypothesis (A and B) in the sample
      byhyp = []
      for hyp in [sample.hyp1,sample.hyp2]:
        # the weights are flattened. rows of iw joined together, last row omitted
        grad_k = np.array([0.0] * ((num_ttables - 1) * num_phrase_features))
        # Iterate through the phrase features
        for j,probs in enumerate(np.transpose(hyp.phrase_scores)):
          # j is phrase feature index
          # probs is phrase-pair, ttable
          grad_jk = np.array([0.0] * (len(iw)-1))
          for l,phi in enumerate(probs):
            # For each phrase-pair the gradient term for the lambda
            # is the probability for this ttable - probability for last ttable
            # divided by overall phrase probability
            num = phi[:-1] - phi[-1]
            denom = np.sum(iw[:,j]*phi) # use interpolation weights for this feature
            grad_jk =  grad_jk + (num/denom)
            self.assertEquals(len(grad_jk), num_ttables-1)
            #print "num",num,"denom",denom,"grad_jk",grad_jk
          # add gradient in correct place
          #print "\n",j,grad_k,phrasew[j]*grad_jk
          grad_k[j*(num_ttables-1):(j+1)*(num_ttables-1)] =\
             grad_k[j*(num_ttables-1):(j+1)*(num_ttables-1)] + phrasew[j]*grad_jk
          #print "\ngrad_k",grad_k
        byhyp.append(grad_k)
      bysample_list.append(byhyp[0]-byhyp[1])
      #print "diff: ", bysample_list[-1]
    expected = np.vstack(bysample_list)  
    actual = self.trainer.gradient_interp(iw,phrasew)
    nptest.assert_almost_equal(actual,expected, decimal=5)

  def test_gradient(self):
    # 2 phrase weights, 2 other feature weights, 
    # 2 interpolation weights (2 models and 2 tables)
    weights = np.array([0.2,0.1,0.4,0.5,0.6,0.3])
    expected = np.array([0.0] * len(weights))
    # Get the gradients 
    iw = np.array([[weights[-2], weights[-1]],
                    [1-weights[-2],1-weights[-1]]]) 
    phrase_g = self.trainer.gradient_phrase(iw)
    other_g = self.trainer.gradient_other()
    interp_g = self.trainer.gradient_interp(iw,weights[:2])
    for k,sample in enumerate(self.samples):
      hyp1_model_score = self.model_score(sample.hyp1, weights)
      hyp2_model_score = self.model_score(sample.hyp2, weights)
      y = 1
      if sample.hyp2.score > sample.hyp1.score: y = -1
      delta_score = hyp1_model_score - hyp2_model_score
      sig_delta_score = sigmoid(-y * delta_score)
      # phrase derivative term
      expected[:2] -= (phrase_g[k]*sig_delta_score*y)
      # other derivative term
      expected[2:4] -= (other_g[k]*sig_delta_score*y)
      # inter derivative term
      expected[-2:] -= (interp_g[k]*sig_delta_score*y)
    expected += self.trainer.alpha*np.append(weights[:-2], np.array([0.0,0.0]))
    actual = self.trainer.gradient(weights)
    nptest.assert_almost_equal(actual,expected)

  def test_split_weights(self):
    w = np.array([1,2,3,4,0.2,0.3])
    sw = self.trainer.get_split_weights(w)
    self.assertEquals(len(sw),3)
    nptest.assert_almost_equal(sw['phrase'], np.array([1,2]))
    nptest.assert_almost_equal(sw['other'], np.array([3,4]))
    nptest.assert_almost_equal(sw['interp'], \
      np.array([[0.2,0.3], [0.8,0.7]]))

  
  def test_train(self):
    """Simple test that it runs without errors"""
    print "x=",self.trainer.train()

if __name__ == "__main__":
  unittest.main()

suite = unittest.TestSuite([
  unittest.TestLoader().loadTestsFromTestCase(TestParabaloidOptimiser),
  unittest.TestLoader().loadTestsFromTestCase(TestLogisticRegressionOptimiser),
  unittest.TestLoader().loadTestsFromTestCase(TestMixtureModelTrainer)])


