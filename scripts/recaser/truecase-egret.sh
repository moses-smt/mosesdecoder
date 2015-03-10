#!/usr/bin/env sh

$1 \
  -input_format egret \
  -output_format egret \
  -no_egret_weight_normalization \
  -case true:model=$3
