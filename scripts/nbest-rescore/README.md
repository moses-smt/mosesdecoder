# N-best List Re-Scorer

Written by Michael Denkowski

These scripts simplify running N-best re-ranking experiments with Moses.  You
can score N-best lists with external tools (such as models that would be very
costly to integrate with Moses just for feasibility experiments), then use the
extended feature set to select translations that may be of a higher quality than
those preferred by the Moses features alone.  In some cases, training a
re-ranker even without any new features can yield improvement.

### Training

* Use Moses to generate large N-best lists for a dev set.  Use a config file
(moses.ini) that has been optimized with MERT, MIRA, or similar:

```
cat dev-src.txt |moses -f moses.ini -n-best-list dev.best1000.out 1000 distinct
```

* (Optionally) add new feature scores to the N-best list using any external
tools.  Make sure the features are added to the correct field using the correct
format.  You don't need to update the final scores (right now your new features
have zero weight):

```
0 ||| some translation ||| Feature0= -1.75645 Feature1= -1.38629 -2.19722 -2.31428 -0.81093 AwesomeNewFeature= -1.38629 ||| -4.42063
```

* Run the optimizer (currently K-best MIRA) to learn new re-ranking weights for
all features in your N-best list.  Supply the reference translation for the dev
set:

```
python train.py --nbest dev.best1000.with-new-features --ref dev-ref.txt --working-dir rescore-work
```

* You now have a new config file that contains N-best re-scoring weights:

```
rescore-work/rescore.ini
```

### Test

* Use the **original** config file to generate N-best lists for the test set:

```
cat test-src.txt |moses -f moses.ini -n-best-list test.best1000.out 100 distinct
```

* Add any new features you added for training

* Re-score the N-best list (update total scores) using the **re-scoring**
weights file:

```
python rescore.py rescore-work/rescore.ini <test.best1000.with-new-features >test.best1000.rescored
```

* The N-best list is **not** re-sorted, so the entries will be out of order.
Use the top-best script to extract the highest scoring entry for each sentence:

```
python topbest.py <test.best1000.rescored >test.topbest
```

### Not implemented yet

The following could be relatively easily implemented by replicating the
behavior of mert-moses.pl:

* Sparse features (sparse weight file)

* Other optimizers (MERT, PRO, etc.)

* Other objective functions (TER, Meteor, etc.)

* Multiple reference translations
