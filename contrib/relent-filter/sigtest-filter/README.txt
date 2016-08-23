Re-implementation of Johnson et al. (2007)'s phrasetable filtering strategy.

This implementation relies on Joy Zhang's SALM Suffix Array toolkit. It is
available here:

  http://projectile.sv.cmu.edu/research/public/tools/salm/salm.htm
  
--Chris Dyer <redpony@umd.edu>

BUILD INSTRUCTIONS
---------------------------------

1. Download and build SALM.

2. make SALMDIR=/path/to/SALM


USAGE INSTRUCTIONS
---------------------------------

1. Using the SALM/Bin/Linux/Index/IndexSA.O32, create a suffix array index
   of the source and target sides of your training bitext.

2. cat phrase-table.txt | ./filter-pt -e TARG.suffix -f SOURCE.suffix \
    -l <FILTER-VALUE>

   FILTER-VALUE is the -log prob threshold described in Johnson et al.
     (2007)'s paper.  It may be either 'a+e', 'a-e', or a positive real
     value. 'a+e' is a good setting- it filters out <1,1,1> phrase pairs.
     I also recommend using -n 30, which filteres out all but the top
     30 phrase pairs, sorted by P(e|f).  This was used in the paper.

3. Run with no options to see more use-cases.


REFERENCES
---------------------------------

H. Johnson, J. Martin, G. Foster and R. Kuhn. (2007) Improving Translation
  Quality by Discarding Most of the Phrasetable. In Proceedings of the 2007
  Joint Conference on Empirical Methods in Natural Language Processing and
  Computational Natural Language Learning (EMNLP-CoNLL), pp. 967-975.
