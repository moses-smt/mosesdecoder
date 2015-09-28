#!/usr/bin/env python
# -*- coding: utf-8 -*-

from wrapper import NMTWrapper
import sys
# arg 1: path to state
# arg 2: path to model
# arg 3: path to source vocab file
# arg 4: path to target vocab file

wr = NMTWrapper(*sys.argv[1:])

wr.build()

c = wr.get_context_vector("this is a little test .")

print wr.get_vec_log_probs(["das"], c, [""])
