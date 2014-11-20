`combine-ptables.pl`: fill-up and other techniques of translation models combination.

Author: 
Arianna Bisazza bisazza[AT]fbk.eu

ABOUT
-----
This tool implements "fill-up" and other operations that are useful to combine translation and reordering tables.
In the "fill-up" approach, the weights of out-domain data sources are estimated directly by MERT along with the 
other model weights.

This tool also supports linear interpolation, but weights must be provided by the user.
If you want to automatically estimate linear interpolation weights, use `contrib/tmcombine` instead.


REFERENCE
---------
When using this script, please cite: 
Arianna Bisazza, Nick Ruiz, and Marcello Federico. 2011. 
"Fill-up versus Interpolation Methods for Phrase-based SMT Adaptation."
In International Workshop on Spoken Language Translation (IWSLT), San Francisco, CA.


FILL-UP
-------

This combination technique is useful when the relevance of the models is known a priori,
e.g. when one is trained on in-domain data and the others on out-of-domain data.

This mode preserves all the entries and scores coming from the first model, and adds
entries from the other models only if new.
If more than two tables are provided, each entry is taken only from the first table 
that contains it.

Moreover, a binary feature is added for each additional table to denote the provenance
of an entry. For in-domain entries, the binary features are all set to 1 (=exp(0)).
Entries coming from the 2nd table will have the 1st binary feature set to 2.718 (=exp(1)).

This technique was proposed in the following works:

Preslav Nakov. 2008. 
"Improving English-Spanish Statistical Machine Translation: Experiments in Domain 
Adaptation, Sentence Paraphrasing, Tokenization, and Recasing."
In Workshop on Statistical Machine Translation.

Arianna Bisazza, Nick Ruiz, and Marcello Federico. 2011. 
"Fill-up versus Interpolation Methods for Phrase-based SMT Adaptation."
In International Workshop on Spoken Language Translation (IWSLT), San Francisco, CA.

The latter paper contains details about the present implementation as well as an empirical
evaluation of fill-up against other combination techniques.
Reordering model fill-up, cascaded fill-up and pruning criteria are also discussed in the 
same paper.

Among the findings of this paper, pruning new (out-of-domain) phrases with more than 4
source words appeared to be beneficial on the Arabic-English TED task when combining the
in-domain models with MultiUn models.
This corresponds to the option:
   `--newSourceMaxLength=4`


BACKOFF
-------

This combination technique is a simplified version of the fill-up technique.
With respect to fill-up technique, the backoff technique does not add the 
binary additional feature denoting the provenance of an entry.


LINEAR INTERPOLATION
--------------------

This combination technique consists in linearly combining the feature values coming
from all tables. The combination weights should be provided by the user, otherwise
uniform weights are assumed.
When a phrase pair is absent from a table, a constant value (epsilon) is assumed for 
the corresponding feature values. You may want to set your own epsilon.

See [Bisazza et al. 2011] for an empirical comparison of uniformly weighted linear 
interpolation against fill-up and decoding-time log-linear interpolation. In that paper, 
epsilon was always set to 1e-06.


UNION
-----

This combination technique creates the union of all phrase pairs and assigns to each
of them the concatenation of all tables scores. 


INTERSECTION
------------

This combination technique creates the intersection of all phrase pairs: each phrase 
pair that occurs in all phrase tables is output along with the feature vector taken 
from the *first* table.
The intersection can be used to prune the reordering table in order to match the 
entries of a corresponding pruned phrase table.


USAGE
-----

Get statistics about overlap of entries:
    `combine-ptables.pl --mode=stats ptable1 ptable2 ... ptableN > ptables-overlap-stats`

Interpolate phrase tables...
- with uniform weights:
    `combine-ptables.pl --mode=interp --phpenalty-at=4 ptable1 ptable2 ptable3 > interp-ptable.X`

- with custom weights:
    `combine-ptables.pl --mode=interp --phpenalty-at=4 --weights=0.8,0.1,0.1 ptable1 ptable2 ptable3 > interp-ptable.Y`

- with custom epsilon:
    `combine-ptables.pl --mode=interp --phpenalty-at=4 --epsilon=1e-05 ptable1 ptable2 ptable3 > interp-ptable.Z`


Fillup phrase tables...
- unpruned:
    `combine-ptables.pl --mode=fillup ptable1 ptable2 ... ptableN > fillup-ptable`

- pruned (new phrases only with max. 4 source words):
    `combine-ptables.pl --mode=fillup --newSourceMaxLength=4 ptable1 ptable2 ... ptableN > fillup-ptable`


Given a pruned phrase table, prune the corresponding reordering table:
    `combine-ptables.pl --mode=intersect1 reotable1-unpruned ptable1-pruned > reotable1-pruned`


NOTES
-----

The script works only with textual (non-binarized) phrase or reordering tables 
that were *previously sorted* with `LC_ALL=C sort`

The resulting combined tables are also textual and need to binarized normally.

The script combine-ptables.pl can be used on lexicalized reordering tables as well.

Input tables can be gzipped.

When integrating filled up models into a Moses system, remember to:
 - specify the correct number of features (typically 6) under [ttable-file] in the configuration file `moses.ini`
 - add a weight under [weight-t] in `moses.ini`
 - if you binarize the models, provide the correct number of features to the command:
    `$moses/bin/processPhraseTable -ttable 0 0 - -nscores $nbFeatures`

