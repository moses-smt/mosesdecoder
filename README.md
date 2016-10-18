# moses decoder with pipelined language model
This fork of Moses project adds pipelined evaluation of language model scores of translation hypotheses. Most important changes are in the files:

- moses/HypothesisStackCubePruningPipelined.cpp
- pipeline/lm/automaton.hh

Pipelined LM runs 1.35x faster for large n-gram models. Translation speed in Moses improves by factor of 1.05x.

More details can be found [here](jmokry.com/pipelineddecoder.pdf)



