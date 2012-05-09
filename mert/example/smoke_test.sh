#!/bin/sh
# A sample script for smoke testing.
# This is not tuning script.
# Please see: mosesdecoder/scripts/training/mert-moses.pl

extractor=../extractor
mert=../mert

# Default the dimension used in mert.
size=15

# Make sure you have already compiled mert related stuff.
for f in $extractor $mert; do
    if ! [ -f $f ]; then
        echo "Error: no such file or directory: $f"
        echo "You should run `bjam` first!"
        exit 1
    fi
done

# Make sure you have sample data and inifile used in this tests.
for f in NBEST REF.0 REF.1 REF.2 init.opt; do
    if ! [ -f $f ]; then
        echo "Error: no such file or directory: $f"
        exit 1
    fi
done

# Read an nbest file, Print output in text format.
# We will save stderr to disk. Please see each log file.
echo "Running tests for reading text files ..."
./normal_test.sh $extractor $mert $size

# Run reading gzipped file tests.
# We will save stderr to disk. Please see each log file.
echo "Running tests for reading gzipped files ..."
./gzipped_test.sh $extractor $mert $size

echo "Smoke tests done."
