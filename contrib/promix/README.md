promix - for training translation model interpolation weights using PRO

Author: Barry Haddow <bhaddow [AT] inf.ed.ac.uk>

ABOUT
-----

The code here provides the "inner loop" for a batch tuning algorithm (like MERT) which 
optimises phrase table interpolation weights at the same time as the standard linear
model weights. Interpolation of the phrase tables uses the "naive" method of tmcombine.

Currently the interpolation only works for two phrase tables, but will soon
be extended to work for more than two.


REQUIREMENTS
------------
The scripts require the Moses Python interface (in contrib/python). It should be built
first, following the instructions in that directory. 

The scripts also require scipy and numpy. They have been tested with the following versions:

*  Python 2.7
*  Scipy 0.11.0
*  Numpy 1.6.2

Run the test.py script to check that everything is functioning correctly.


USAGE
-----
Since the code in this directory provides the inner loop for a batch tuning algorithm,
it is run from the increasingly inaccurately named mert-moses.pl. If you want to run
the optimiser directly, run `main.py -h` for usage.

A sample command for mert-moses.pl is as follows:

    MOSES/scripts/training/mert-moses.pl \
       input-file ref-file \
       decoder  \
       ini-file  \
        --promix-training MOSES/contrib/promix/main.py \
        --maximum-iterations 15 \
        --promix-table phrase-table-1 \
        --promix-table phrase-table-2 \
        --filtercmd "MOSES/scripts/training/filter-model-given-input.pl --Binarizer MOSES/bin/processPhraseTable" \
        --nbest 100 --working-dir ./tmp  --decoder-flags "-threads 4 -v 0 " \
        --rootdir MOSES/scripts -mertdir MOSES/bin \
        --return-best-dev 

Note that promix training requires a filter and binarise script, and that the phrase table
referenced in the ini file is not used. The argument `--return-best-dev` is not essential,
but recommended.


REFERENCES
----------

The code here was created for:

Haddow, Barry (2013) Applying Pairwise Ranked Optimisation to
Improve the Interpolation of Translation Models. In: Proceedings of NAACL 2013

See also:

Sennrich, Rico (2012). Perplexity Minimization for Translation Model Domain Adaptation in Statistical Machine Translation. In: Proceedings of EACL 2012.
