#!/usr/bin/env python

import logging
import optparse
import subprocess
import sys

def main():
  logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', datefmt='%Y-%m-%d %H:%M:%S', level=logging.DEBUG)
  parser = optparse.OptionParser("%prog [options]")
  parser.add_option("-w", "--working-dir", dest="working_dir")
  parser.add_option("-c", "--corpus", dest="corpus_stem")
  parser.add_option("-l", "--nplm-home", dest="nplm_home")
  parser.add_option("-e", "--epochs", dest="epochs", type="int")
  parser.add_option("-n", "--ngram-size", dest="ngram_size", type="int")
  parser.add_option("-b", "--minibatch-size", dest="minibatch_size", type="int")
  parser.add_option("-s", "--noise", dest="noise", type="int")
  parser.add_option("-d", "--hidden", dest="hidden", type="int")
  parser.add_option("-i", "--input-embedding", dest="input_embedding", type="int")
  parser.add_option("-o", "--output-embedding", dest="output_embedding", type="int")
  parser.add_option("-t", "--threads", dest="threads", type="int")
  parser.add_option("-m", "--output-model", dest="output_model")

  parser.set_defaults(
    working_dir = "working"
    ,corpus_stem = "train.10k"
    ,nplm_home = "/home/bhaddow/tools/nplm"
    ,epochs = 10
    ,ngram_size = 14
    ,minibatch_size=1000
    ,noise=100
    ,hidden=750
    ,input_embedding=150
    ,output_embedding=150
    ,threads=8
    ,output_model = "train.10k"
  )

  options,args = parser.parse_args(sys.argv)

  in_file = options.working_dir + "/" + options.corpus_stem + ".ngrams"
  vocab_file = options.working_dir + "/vocab"
  prep_file = options.working_dir + "/" + options.output_model + ".prepared"

  prep_args = [options.nplm_home + "/src/prepareNeuralLM", "--train_text", in_file, "--ngram_size", \
                str(options.ngram_size), "--ngramize", "0", "--words_file", vocab_file, "--train_file", prep_file ]
  print "Prepare model command: "
  print ', '.join(prep_args)

  ret = subprocess.call(prep_args)
  if ret: raise Exception("Prepare failed")

  model_prefix = options.working_dir + "/" + options.output_model + ".model.nplm"
  train_args = [options.nplm_home + "/src/trainNeuralNetwork", "--train_file", prep_file, "--num_epochs", str(options.epochs),
                "--input_words_file", vocab_file, "--output_words_file", vocab_file, "--model_prefix",
                model_prefix, "--learning_rate", "1", "--minibatch_size", str(options.minibatch_size),
                "--num_noise_samples", str(options.noise), "--num_hidden", str(options.hidden), "--input_embedding_dimension",
                str(options.input_embedding), "--output_embedding_dimension", str(options.output_embedding), "--num_threads",
                str(options.threads)]
  print "Train model command: "
  print ', '.join(train_args)

  ret = subprocess.call(train_args)
  if ret: raise Exception("Training failed")

if __name__ == "__main__":
  main()




#EPOCHS=10
#NGRAM_SIZE=14
#MINIBATCH_SIZE=1000
#NOISE=100
#HIDDEN=750
#INPUT_EMBEDDING=150
#OUTPUT_EMBEDDING=150
#THREADS=8
#

#$ROOT/src/prepareNeuralLM --train_text $INFILE --ngram_size $NGRAM_SIZE --ngramize 0  --words_file $VOCAB --train_file $WORKDIR/train.ngrams || exit 1

#$ROOT/src/trainNeuralNetwork --train_file $WORKDIR/train.ngrams  \
#   --num_epochs $EPOCHS --input_words_file $VOCAB --output_words_file $VOCAB --model_prefix $WORKDIR/$PREFIX \
#   --learning_rate 1 --minibatch_size $MINIBATCH_SIZE --num_noise_samples $NOISE --num_hidden $HIDDEN \
#   --input_embedding_dimension $INPUT_EMBEDDING --output_embedding_dimension $OUTPUT_EMBEDDING --num_threads $THREADS || exit 1


