#!/usr/bin/env python

import logging
import optparse
import subprocess
import sys
import os

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
  parser.add_option("-r", "--output-dir", dest="output_dir")
  parser.add_option("-f", "--config-options-file", dest="config_options_file")
  parser.add_option("-g", "--log-file", dest="log_file")
  parser.add_option("-v", "--validation-ngrams", dest="validation_file")
  parser.add_option("-a", "--activation-function", dest="activation_fn")
  parser.add_option("-z", "--learning-rate", dest="learning_rate")

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
    ,threads=1
    ,output_model = "train.10k"
    ,output_dir = None
    ,config_options_file = "config"
    ,log_file = "log"
    ,validation_file = None
    ,activation_fn = "rectifier"
    ,learning_rate = "1"
  )

  options,args = parser.parse_args(sys.argv)
  
  # Set up validation command variable to use with validation set.
  validations_command = []
  if options.validation_file is not None:
    validations_command =["--validation_file", (options.validation_file + ".numberized")]
    

  # In order to allow for different models to be trained after the same
  # preparation step, we should provide an option for multiple output directories
  # If we have not set output_dir, set it to the same thing as the working dir

  if options.output_dir is None:
    options.output_dir = options.working_dir
  else:
    # Create output dir if necessary
    if not os.path.exists(options.output_dir):
      os.makedirs(options.output_dir)

  config_file = options.output_dir + "/" + options.config_options_file + '-' + options.output_model
  log_file = options.output_dir + "/" + options.log_file + '-' + options.output_model
  log_file_write = open(log_file, 'w')
  config_file_write = open(config_file, 'w')

  config_file_write.write("Called: " + ' '.join(sys.argv) + '\n\n')

  in_file = options.working_dir + "/" + options.corpus_stem + ".numberized"
      

  model_prefix = options.output_dir + "/" + options.output_model + ".model.nplm"
  train_args = [options.nplm_home + "/src/trainNeuralNetwork", "--train_file", in_file, "--num_epochs", str(options.epochs),
                "--model_prefix",
                model_prefix, "--learning_rate", options.learning_rate, "--minibatch_size", str(options.minibatch_size),
                "--num_noise_samples", str(options.noise), "--num_hidden", str(options.hidden), "--input_embedding_dimension",
                str(options.input_embedding), "--output_embedding_dimension", str(options.output_embedding), "--num_threads",
                str(options.threads), "--activation_function", options.activation_fn] + validations_command
  print "Train model command: "
  print ', '.join(train_args)

  config_file_write.write("Training step:\n" + ' '.join(train_args) + '\n')
  config_file_write.close()

  log_file_write.write("Training output:\n")
  ret = subprocess.call(train_args, stdout=log_file_write, stderr=log_file_write)
  if ret: raise Exception("Training failed")

  log_file_write.close()

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


