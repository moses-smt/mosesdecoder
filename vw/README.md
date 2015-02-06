Vowpal Wabbit for Moses
=======================

This is an attempt to integrate Vowpal Wabbit with Moses as a stateless feature
function.

Compatible with this frozen version of VW:

    https://github.com/moses-smt/vowpal_wabbit
    
To enable VW, you need to provide a path where VW was installed (using `make install`) to bjam:

    ./bjam --with-vw=<path/to/vw/installation>

Implemented classifier features
-------------------------------

* `VWFeatureSourceBagOfWords`: This creates a feature of form bow^token for every
source sentence token.
* `VWFeatureSourceExternalFeatures column=0`: when used with -inputtype 5 (`TabbedSentence`) this can be used to supply additional feature to VW. The input is a tab-separated file, the first column is the usual input sentence, all other columns can be used for meta-data. Parameter column=0 counts beginning with the first column that is not the input sentence.  
* `VWFeatureSourceIndicator`: Ass a feature for the whole source phrase.
* `VWFeatureSourcePhraseInternal`: Adds a separate feature for every word of the source phrase.
* `VWFeatureSourceWindow size=3`: Adds source words in a window of size 3 before and after the source phrase as features. These do not overlap with `VWFeatureSourcePhraseInternal`.
* `VWFeatureTargetIndicator`: Adds a feature for the whole target phrase.
* `VWFeatureTargetPhraseInternal`: Adds a separate feature for every word of the target phrase.

Configuration
-------------

To use the classifier edit your moses.ini

    [features]
    ...
    VW path=/home/username/vw/classifier1.vw
    VWFeatureSourceBagOfWords
    VWFeatureTargetIndicator
    VWFeatureSourceIndicator
    ...
     
    [weights]
    ...
    VW0= 0.2
    ...

If you change the name of the main VW feature, remember to tell the VW classifier
features which classifier they belong to:

    [features]
    ...
    VW name=bart path=/home/username/vw/classifier1.vw 
    VWFeatureSourceBagOfWords used-by=bart
    VWFeatureTargetIndicator used-by=bart
    VWFeatureSourceIndicator used-by=bart
    ...
    
    [weights]
    ...
    bart= 0.2
    ...

You can also use multiple classifiers:

    [features]
    ...
    VW name=bart path=/home/username/vw/classifier1.vw 
    VW path=/home/username/vw/classifier2.vw
    VW path=/home/username/vw/classifier3.vw
    VWFeatureSourceBagOfWords used-by=bart,VW0 
    VWFeatureTargetIndicator used-by=VW1,VW0,bart
    VWFeatureSourceIndicator used-by=bart,VW1
    ...
    
    [weights]
    ...
    bart= 0.2
    VW0= 0.2
    VW1= 0.2
    ...

Features can use any combination of factors. Provide a comma-delimited list of factors in the `source-factors` or `target-factors` variables to override the default setting (`0`, i.e. the first factor).
    
Training the classifier
-----------------------

Training uses `vwtrainer` which is a limited version of the `moses` binary. To train, provide your training data as input in the following format:

    source tokens<tab>target tokens<tab>word alignment

Use Moses format for the word alignment (`0-0 1-0` etc.). Set the input type to 5 (`TabbedSentence`, see above):

    [inputtype]
    5

Configure your features in the `moses.ini` file (see above) and set the `train` flag:

     [features]
     ... 
     VW name=bart path=/home/username/vw/features.txt train=1
     ...

The `path` variable points to the file (prefix) where features will be written. Currently, threads write to separate files (maybe subject to change sooner or later): `features.txt.1`, `features.txt.2` etc.

`vwtrainer` creates the translation option collection for each input sentence but does not run decoding. Therefore, you probably want to disable expensive feature functions such as the language model (LM score is not used by VW features at the moment).

Run `vwtrainer`:

    vwtrainer -f moses.trainvw.ini < tab-separated-training-data.tsv

Currently, classification is implemented using VW's `csoaa_ldf` scheme with quadratic features which take the product of the source namespace (`s`, contains label-independent features) and the target namespace (`t`,  contains label-dependent features).

To train VW in this setting, use the command:

    cat features.txt.* | vw --hash all --loss_function logistic --noconstant -b 26 -q st --csoaa_ldf mc -f classifier1.vw
