tmcombine - a tool for Moses translation model combination

Author: Rico Sennrich <sennrich [AT] cl.uzh.ch>

ABOUT
-----

This program handles the combination of Moses phrase tables, either through
linear interpolation of the phrase translation probabilities/lexical weights,
or through a recomputation based on the (weighted) combined counts.

It also supports an automatic search for weights that minimize the cross-entropy
between the model and a tuning set of word/phrase alignments.


REQUIREMENTS
------------

The script requires Python >= 2.6.
SciPy is recommended. If it is missing, an ad-hoc hill-climbing optimizer will be used (which may be slower, but is actually recommended for PyPy and/or a high number of models).
On Debian-based systems, you can install SciPy from the repository:
    sudo apt-get install python-scipy


USAGE
-----

for usage information, run
    ./tmcombine.py -h

Two basic command line examples:

linearly interpolate two translation models with fixed weights:
    ./tmcombine.py combine_given_weights test/model1 test/model2 -w "0.1,0.9;0.1,1;0.2,0.8;0.5,0.5" -o test/phrase-table_test2

do a count-based combination of two translation models with weights that minimize perplexity on a set of reference phrase pairs.
    ./tmcombine.py combine_given_tuning_set test/model1 test/model2 -o test/phrase-table_test5 -m counts -r test/extract

Typically, you have to specify one action out of the following:

 - `combine_given_weights`: write a new phrase table with defined weights

 - `combine_given_tuning_set`: write a new phrase table, using the weights that minimize cross-entropy on a tuning set

 - `compare_cross_entropies`: print cross-entropies for each model/feature, using the intersection of phrase pairs.

 - `compute_cross_entropy`: return cross-entropy for a tuning set, a set of models and a set of weights.

 - `return_best_cross_entropy`: return the set of weights and cross-entropy that is optimal for a tuning set and a set of models.

You can check the docstrings of `Combine_TMs()` for more information and find some example commands in the function `test()`.
Some configuration options (i.e. normalization of linear interpolation) are not accessible from the command line.
You can gain a bit more flexibility by writing/modifying python code that initializes `Combine_TMs()` with your desired arguments, or by just fiddling with the default values in the script.

Regression tests (check if the output files (`test/phrase-table_testN`) differ from the files in the repositorys):
    ./tmcombine.py test

FURTHER NOTES
-------------

 - Different combination algorithms require different statistics. To be on the safe side, use the option and `-write-lexical-counts` when training models.

 - The script assumes that phrase tables are sorted (to allow incremental, more memory-friendly processing). Sort the tables with `LC_ALL=C`. Phrase tables produced by Moses are sorted correctly.

 - Some configurations require additional statistics that are loaded in memory (lexical tables; complete list of target phrases). 
   If memory consumption is a problem, use the option --lowmem (slightly slower and writes temporary files to disk), or consider pruning your phrase table before combining (e.g. using Johnson et al. 2007).

 - The script can read/write gzipped files, but the Python implementation is slow. You're better off unzipping the files on the command line and working with the unzipped files. The script will automatically search for the unzipped file first, and for the gzipped file if the former doesn't exist.

 - The cross-entropy estimation assumes that phrase tables contain true probability distributions (i.e. a probability mass of 1 for each conditional probability distribution). If this is not true, the results may be skewed.

 - Unknown phrase pairs are not considered for the cross-entropy estimation. A comparison of models with different vocabularies may be misleading.

 - Don't directly compare cross-entropies obtained from a combination with different modes. Depending on how some corner cases are treated, linear interpolation does not distribute the full probability mass and thus shows higher (i.e. worse) cross-entropies.


REFERENCES
----------

The algorithms are described in

Sennrich, Rico (2012). Perplexity Minimization for Translation Model Domain Adaptation in Statistical Machine Translation. In: Proceedings of EACL 2012.

The evaluated algorithms are:

 - linear interpolation (naive): default
 - linear interpolation (modified): use options `--normalized` and `--recompute_lexweights`
 - weighted counts: use option `-m counts`
