#!/bin/sh

# merges subword units that were split by BPE

sed -r 's/\@\@ //g'