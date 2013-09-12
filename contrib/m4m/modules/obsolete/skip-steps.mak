# -*- Makefile -*-

# Specify in this file resources that you already have
run_id ?= 0

untuned_moses_ini    := model/moses.ini.0
moses_ini_for_tuning  = ${untuned_moses_ini} 
moses_ini_for_eval    = ${tuned_moses_ini} 

# Notes: 
# 
# - if ${moses_ini_for_tuning} is different from ${untuned_mose_ini}, the phrase table and the
#   lexical distortion table will be filtered for tuning (see tune.make)
# - if ${moses_ini_for_eval} is different from ${tuned_mose_ini}, the phrase table and the
#   lexical distortion table will be filtered for evaluation (see eval.make)


all:
	echo ";$(foo);"
