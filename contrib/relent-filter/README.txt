Implementation of the Relative Entropy-based Phrase table filtering algorithm by Wang Ling (Ling et al, 2012).

This implementation also calculates the significance scores for the phrase tables based on the Fisher's Test(Johnson et al, 2007). Uses a slightly modified version of the "sigtest-filter" by Chris Dyer. 

-------BUILD INSTRUCTIONS-------

1 - Build the sigtest-filter binary

1.1 - Download and build SALM available at http://projectile.sv.cmu.edu/research/public/tools/salm/salm.htm

1.2 - Run "make SALMDIR=<path_to_salm>" in "<path_to_moses>/contrib/relent-filter/sigtest-filter" to create the executable filter-pt

2 - Build moses project by running "./bjam <options>", this will create the executables for relent filtering 

-------USAGE INSTRUCTIONS-------

Required files:
s_train - source training file 
t_train - target training file
moses_ini - path to the moses configuration file ( after tuning )
pruning_binaries - path to the relent pruning binaries ( should be "<path_to_moses>/bin" )
pruning_scripts - path to the relent pruning scripts ( should be "<path_to_moses>/contrib/relent-filter/scripts" )
sigbin - path to the sigtest filter binaries ( should be "<path_to_moses>/contrib/relent-filter/sigtest-filter" )
output_dir - path to write the output

1 - build suffix arrays for the source and target parallel training data

1.1 - run "<path to salm>/Bin/Linux/Index/IndexSA.O32 <s_train>" (or IndexSA.O64)

1.2 - run "<path to salm>/Bin/Linux/Index/IndexSA.O32 <t_train>" (or IndexSA.O64)

2 - calculate phrase pair scores by running:

perl <pruning_scripts>/calcPruningScores.pl -moses_ini <moses_ini> -training_s <s_train> -training_t <t_train> -prune_bin <pruning_binaries> -prune_scripts <pruning_scripts> -moses_scripts <path_to_moses>/scripts/training/ -workdir <output_dir> -dec_size 10000

this will create the following files in the <output_dir/scores/> dir:

count.txt - counts of the phrase pairs for N(s,t) N(s,*) and N(*,t)
divergence.txt - negative log of the divergence of the phrase pair
empirical.txt - empirical distribution of the phrase pairs N(s,t)/N(*,*)
rel_ent.txt - relative entropy of the phrase pairs
significance.txt - significance of the phrase pairs

You can use any one of these files for pruning and also combine these scores using <pruning_scripts>/interpolateScores.pl

3 - To actually prune a phrase table you should run <pruning_scripts>/prunePT.pl

For instance, to prune 30% of the phrase table using rel_ent run:
perl <pruning_scripts>/prunePT.pl -table <phrase_table_file> -scores <output_dir>/scores/rel_ent.txt -percentage 70 > <pruned_phrase_table_file>

You can also prune by threshold 
perl <pruning_scripts>/prunePT.pl -table <phrase_table_file> -scores <output_dir>/scores/rel_ent.txt -threshold 0.1 > <pruned_phrase_table_file>

The same must be done for the reordering table by replacing <phrase_table_file> with the <reord_table_file>

perl <pruning_scripts>/prunePT.pl -table <reord_table_file> -scores <output_dir>/scores/rel_ent.txt -percentage 70 > <pruned_reord_table_file>

-------RUNNING STEP 2 IN PARALLEL-------

Step 2 requires the forced decoding of the whole set of phrase pairs in the table, so unless you test it on a small corpora, it usually requires large amounts of time to process. 
Thus, we recommend users to run multiple instances of "<pruning_scripts>/calcPruningScores.pl" in parallel to process different parts of the phrase table. 

To do this, run:

perl <pruning_scripts>/calcPruningScores.pl -moses_ini <moses_ini> -training_s <s_train> -training_t <t_train> -prune_bin <pruning_binaries> -prune_scripts <pruning_scripts> -moses_scripts <path_to_moses>/scripts/training/ -workdir <output_dir> -dec_size 10000 -start 0 -end 100000

The -start and -end tags tell the script to only calculate the results for phrase pairs between 0 and 99999. 

Thus, an example of a shell script to run for the whole phrase table would be:

size=`wc <phrase_table_file> | gawk '{print $1}'`
phrases_per_process=100000

for i in $(seq 0 $phrases_per_process $size)
do
   end=`expr $i + $phrases_per_process`
   perl <pruning_scripts>/calcPruningScores.pl -moses_ini <moses_ini> -training_s <s_train> -training_t <t_train> -prune_bin <pruning_binaries> -prune_scripts <pruning_scripts> -moses_scripts <path_to_moses>/scripts/training/ -workdir <output_dir>.$i-$end -dec_size 10000 -start $i -end $end
done

After all processes finish, simply join the partial score files together in the same order.

-------REFERENCES-------
Ling, W., Graça, J., Trancoso, I., and Black, A. (2012). Entropy-based pruning for phrase-based
machine translation. In Proceedings of the 2012 
Joint Conference on Empirical Methods in Natural Language Processing and
Computational Natural Language Learning (EMNLP-CoNLL), pp. 962-971.

H. Johnson, J. Martin, G. Foster and R. Kuhn. (2007) Improving Translation
Quality by Discarding Most of the Phrasetable. In Proceedings of the 2007
Joint Conference on Empirical Methods in Natural Language Processing and
Computational Natural Language Learning (EMNLP-CoNLL), pp. 967-975.
